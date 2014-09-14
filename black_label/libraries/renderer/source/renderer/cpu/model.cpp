#define BLACK_LABEL_SHARED_LIBRARY_EXPORT
#include <black_label/renderer/cpu/model.hpp>

#include <algorithm>
#include <fstream>
#include <queue>
#include <vector>

#include <fstream>

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/log/trivial.hpp>

#include <GL/glew.h>
#ifndef APPLE
#include <GL/gl.h>
#else
#include <OpenGL/gl.h>
#endif

#ifdef DEVELOPER_TOOLS

#ifndef NO_FBX
#include <fbxsdk.h>
#include <fbxsdk/fileio/fbxiosettings.h>

#include <queue>
#include <type_traits>

#include <boost/optional.hpp>
#endif

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#endif // #ifdef DEVELOPER_TOOLS



using namespace black_label::utility;
using namespace std;
using namespace boost;
using namespace boost::archive;



namespace black_label {
namespace renderer {
namespace cpu {



bool model::import( path path )
{
	if (cache_file::import(path, *this))
		return true;
#ifdef DEVELOPER_TOOLS
	if (
#ifndef NO_FBX
//		(".fbx" == path.extension() && import_fbxsdk(path)) || 
#endif
		import_assimp(path))
		if (cache_file::export(path, *this))
			return true;
#endif // #ifdef DEVELOPER_TOOLS

	BOOST_LOG_TRIVIAL(warning) << "Failed to import model " << path;
	return false;
}



#ifdef DEVELOPER_TOOLS



////////////////////////////////////////////////////////////////////////////////
/// Elementwise
////////////////////////////////////////////////////////////////////////////////
struct default_type {};

template<int count, typename cast = default_type, typename iterator_type = void>
class elementwise_iterator : public std::iterator<
	typename iterator_type::iterator_category, 
	typename iterator_type::value_type,
	typename iterator_type::difference_type, 
	typename iterator_type::pointer,
	typename iterator_type::reference>
{
public:
	elementwise_iterator( iterator_type iterator ) : iterator{iterator} {}
	elementwise_iterator& operator*() { return *this; }
	template<typename vector_type>
	void operator=( vector_type vector )
	{ 
		using cast_type = typename conditional<
			std::is_same<default_type, cast>::value, 
			typename decltype(vector[int{0}]), 
			cast>::type;

		for (int i{0}; count > i; ++i) 
			*iterator++ = static_cast<cast_type>(vector[i]);
	}
	elementwise_iterator& operator++() { ++iterator; return *this; }
	elementwise_iterator operator++( int ) { auto before = *this; iterator++; return std::move(before); }

	iterator_type iterator;
};

template<int count, typename cast = default_type, typename iterator_type = void>
elementwise_iterator<count, cast, iterator_type> elementwise( iterator_type iterator )
{ return elementwise_iterator<count, cast, iterator_type>{iterator}; }



////////////////////////////////////////////////////////////////////////////////
/// Apply
////////////////////////////////////////////////////////////////////////////////
template<typename callable_type, typename iterator_type>
class apply_iterator : public std::iterator<
	typename iterator_type::iterator_category, 
	typename iterator_type::value_type,
	typename iterator_type::difference_type, 
	typename iterator_type::pointer,
	typename iterator_type::reference>
{
public:
	apply_iterator( callable_type callable, iterator_type iterator ) : callable{callable}, iterator{iterator} {}
	apply_iterator& operator*() { return *this; }
	template<typename T> 
	void operator=( T value ) { *iterator = callable(value); }
	apply_iterator& operator++() { ++iterator; return *this; }
	apply_iterator operator++( int ) { auto before = *this; iterator++; return std::move(before); }

	callable_type callable;
	iterator_type iterator;
};

template<typename callable_type, typename iterator_type>
apply_iterator<callable_type, iterator_type> apply( callable_type callable, iterator_type iterator )
{ return apply_iterator<callable_type, iterator_type>(callable, iterator); }



#ifndef NO_FBX
    
////////////////////////////////////////////////////////////////////////////////
/// Make FBX
////////////////////////////////////////////////////////////////////////////////
template<typename fbx_class>
void destroy_fbx( fbx_class* fbx ) { fbx->Destroy(); }

template<typename fbx_class>
using unique_fbx_ptr = unique_ptr<fbx_class, void (*)( fbx_class* )>;

template<typename fbx_class, typename... T>
unique_fbx_ptr<fbx_class> make_fbx(T... arguments)
{ return unique_fbx_ptr<fbx_class>{fbx_class::Create(arguments...), destroy_fbx}; }



////////////////////////////////////////////////////////////////////////////////
/// Parse Functions
////////////////////////////////////////////////////////////////////////////////
template<typename T, typename iterator>
bool parse_layer_element(
	const FbxLayerElementTemplate<T>* layer_element,
	iterator output_begin )
{
	switch (layer_element->GetMappingMode())
	{
	case FbxLayerElement::eByPolygonVertex:
	case FbxLayerElement::eByControlPoint:
	{
		const auto& direct_array = layer_element->GetDirectArray();
		const auto direct_array_count = direct_array.GetCount();

		switch (layer_element->GetReferenceMode())
		{
		case FbxLayerElement::eDirect:
		{
			for (int i{0}; direct_array_count > i; ++i)
				*output_begin++ = direct_array[i];
		} break;
				
		case FbxLayerElement::eIndexToDirect:
		{
			const auto& index_array = layer_element->GetIndexArray();
			const auto index_array_count = index_array.GetCount();

			for (int i{0}; index_array_count > i; ++i)
				*output_begin++ = direct_array[index_array[i]];
		} break;

		default:
			BOOST_LOG_TRIVIAL(warning) << "Unsupported reference mode";
			return false;
		}
	} break;

	default:
		BOOST_LOG_TRIVIAL(warning) << "Unsupported mapping mode";
		return false;
	}

	// Success
	return true;
}



bool is_mapped_by_control_point( const FbxLayerElement* layer_element )
{ return FbxLayerElement::eByControlPoint == layer_element->GetMappingMode(); }

template<typename T>
int get_layer_element_count( const FbxLayerElementTemplate<T>* layer_element )
{
	switch (layer_element->GetMappingMode())
	{
	case FbxLayerElement::eByPolygonVertex:
	case FbxLayerElement::eByControlPoint:
	{
		switch (layer_element->GetReferenceMode())
		{
		case FbxLayerElement::eDirect:
			return layer_element->GetDirectArray().GetCount();
				
		case FbxLayerElement::eIndexToDirect:
			return layer_element->GetIndexArray().GetCount();

		default:
			BOOST_LOG_TRIVIAL(warning) << "Unsupported reference mode";
			return 0;
		}
	} break;

	default:
		BOOST_LOG_TRIVIAL(warning) << "Unsupported mapping mode";
		return 0;
	}
}



bool parse_normals(
	const FbxMesh* mesh,
	const FbxAMatrix& transform,
	mesh::vector_container& normal_cache,
	bool& indexed )
{
	// Clear cache
	normal_cache.clear();

	// Extract node
	auto node = mesh->GetNode();

	// Count layer elements
	auto layer_element_count = mesh->GetElementNormalCount();

	// No normals
	if (0 >= layer_element_count)
	{
		BOOST_LOG_TRIVIAL(info) << "Mesh has no normals: \"" << node->GetName() << "\"";
		return true;
	}

	// Unused normal layer elements
	if (1 < layer_element_count)
		BOOST_LOG_TRIVIAL(info) << "Mesh has more than 1 normal per vertex: \"" << node->GetName() << "\"Using the first normal and ignoring the rest.";

	// Just use the first layer element (and ignore the rest)
	auto layer_element = mesh->GetElementNormal(0);

	// Set indexed
	if (!is_mapped_by_control_point(layer_element))
		indexed = false;

	// Reserve space
	normal_cache.reserve(get_layer_element_count(layer_element) * 3);

	// iterator
	auto normals_begin = apply([transform]( auto vec4 ){ return transform.MultR(vec4); }, 
		elementwise<3, float>(back_inserter(normal_cache)));

	// Parse layer element
	if (!parse_layer_element(layer_element, normals_begin))
	{
		BOOST_LOG_TRIVIAL(warning) << "Unable to load normals for mesh: \"" << node->GetName() << "\"";
		return false;
	}

	// Success
	return true;
}



bool parse_texture_coordinates(
	const FbxMesh* mesh,
	const FbxAMatrix& transform,
	mesh::vector_container& texture_coordinate_cache,
	bool& indexed )
{
	// Clear cache
	texture_coordinate_cache.clear();

	// Extract node
	auto node = mesh->GetNode();

	// Count layer elements
	auto layer_element_count = mesh->GetElementUVCount();

	// No texture coordinates
	if (0 >= layer_element_count)
	{
		BOOST_LOG_TRIVIAL(info) << "Mesh has no texture coordinates: \"" << node->GetName() << "\"";
		return true;
	}

	// Unused normal layer elements
	if (1 < layer_element_count)
		BOOST_LOG_TRIVIAL(info) << "Mesh has more than 1 texture coordinate per vertex: \"" << node->GetName() << "\"Using the first texture coordinate and ignoring the rest.";

	// Just use the first layer element (and ignore the rest)
	auto layer_element = mesh->GetElementUV(0);

	// Set indexed
	if (!is_mapped_by_control_point(layer_element))
		indexed = false;

	// Reserve space
	texture_coordinate_cache.reserve(get_layer_element_count(layer_element) * 2);

	// Iterator
	auto texture_coordinate_begin = elementwise<2, float>(back_inserter(texture_coordinate_cache));

	// Parse layer element
	if (!parse_layer_element(layer_element, texture_coordinate_begin))
	{
		BOOST_LOG_TRIVIAL(warning) << "Unable to load texture coordinates for mesh: \"" << node->GetName() << "\"";
		return false;
	}

	// Success
	return true;
}



void parse_vertices(
	const FbxMesh* mesh,
	FbxAMatrix transform,
	mesh::vector_container& cache,
	bool indexed )
{
	// Transformation
	auto MultT_global = [&]( FbxVector4 vector4 ){ return transform.MultT(vector4); };

	// Clear cache
	cache.clear();

	// Iterators
	auto vertices_begin = apply(MultT_global, elementwise<3, float>(back_inserter(cache)));
	const auto control_points_begin = mesh->GetControlPoints();

	if (indexed)
	{
		const auto control_points_count = mesh->GetControlPointsCount();
		const auto control_points_end = &control_points_begin[control_points_count];
		cache.reserve(control_points_end - control_points_begin);
		copy(control_points_begin, control_points_end, vertices_begin);
	}
	else
	{
		cache.reserve(3 * mesh->GetPolygonCount());
		for (int i{0}; mesh->GetPolygonCount() > i; ++i)
			for (int j{0}; 3 > j; ++j)
			{
				auto index = mesh->GetPolygonVertex(i, j);
				*vertices_begin++ = control_points_begin[index];
			}
	}
}



void parse_indices(
	const FbxMesh* mesh,
	mesh::index_container& cache,
	bool indexed )
{
	if (!indexed)
	{
		cache.clear();
		return;
	}
	
	auto fbx_indices_count = 3 * mesh->GetPolygonCount();
	auto fbx_indices_begin = mesh->GetPolygonVertices();
	auto fbx_indices_end = &fbx_indices_begin[fbx_indices_count];
	cache.assign(fbx_indices_begin, fbx_indices_end);
}


bool parse_mesh(
	model& model,
	const FbxMesh* mesh, 
	path path,
	mesh::vector_container& vertex_cache, 
	mesh::vector_container& normal_cache,
	mesh::vector_container& texture_coordinate_cache,
	mesh::index_container& index_cache)
{
	// Types
	using vector_value_type = mesh::vector_container::value_type;
	using indices_value_type = mesh::index_container::value_type;

	// Extract node
	auto node = mesh->GetNode();

	// Transformation
	auto& global_transform = node->EvaluateGlobalTransform();
	
	// Meshes must be composed of triangles
	if (!mesh->IsTriangleMesh())
	{
		BOOST_LOG_TRIVIAL(warning) << "Mesh is not composed of triangles: \"" << node->GetName() << "\"";
		return false;
	}

	// Mapping
	bool indexed = true;

	// Element parsing
	if (!parse_normals(mesh, global_transform, normal_cache, indexed) ||
		!parse_texture_coordinates(mesh, global_transform, texture_coordinate_cache, indexed))
		return false;

	parse_vertices(mesh, global_transform, vertex_cache, indexed);
	parse_indices(mesh, index_cache, indexed);



	// Material
	material material;
	material.diffuse = glm::vec3(1.0f);
	material.alpha = 1.0f;

	

	if (1 < mesh->GetElementMaterialCount())
		BOOST_LOG_TRIVIAL(info) << "Mesh has more than 1 material: \"" << node->GetName() 
			<< "\". Using the first material and ignoring the rest.";


	auto fbx_material_element = mesh->GetElementMaterial(0);

	switch (fbx_material_element->GetMappingMode())
	{
	case FbxLayerElement::eAllSame:
	{
		auto index = fbx_material_element->GetIndexArray().GetAt(0);
		auto fbx_material = node->GetMaterial(index);

		auto diffuse_property = fbx_material->FindProperty(FbxSurfaceMaterial::sDiffuse);
		const auto texture_count = diffuse_property.GetSrcObjectCount<FbxTexture>();
		
		if (0 == texture_count) break;

		if (1 < texture_count)
			BOOST_LOG_TRIVIAL(info) << "Mesh has more than 1 diffuse texture: \"" << node->GetName() 
				<< "\". Using the first texture and ignoring the rest.";

		auto texture = (diffuse_property.GetSrcObject<FbxFileTexture>(0));
		material.diffuse_texture = path.parent_path() / texture->GetRelativeFileName();
	} break;

	default:
		BOOST_LOG_TRIVIAL(info) << "Mesh has unsupported mapping mode: \"" << node->GetName() << "\"";
		return false;
	}		

	auto fbx_roughness = node->FindProperty("black_label_roughness", true);
	auto roughness = (fbx_roughness.IsValid()) 
		? static_cast<FbxPropertyT<FbxFloat> >(fbx_roughness).Get()
		: 1.0f;



	model.meshes.emplace_back(
		std::move(material),
		GL_TRIANGLES,
		vertex_cache,
		normal_cache,
		texture_coordinate_cache,
		index_cache);

	// Success
	return true;
}



bool parse_light(
	model& model,
	const FbxLight* fbx_light, 
	const FbxNode* node ) 
{
	auto fbx_color = fbx_light->Color.Get();
	glm::vec4 color{fbx_color[0], fbx_color[1], fbx_color[2], 1.0f};

	auto fbx_position = node->LclTranslation.Get();
	glm::vec3 position{fbx_position[0], fbx_position[1], fbx_position[2]};
	auto fbx_direction = node->LclRotation.Get();
	glm::vec3 direction{fbx_direction[0], fbx_direction[1], fbx_direction[2]};

	model.lights.emplace_back(
		light_type::spot, 
		position, 
		direction, 
		color);

	// Success
	return true;
}



////////////////////////////////////////////////////////////////////////////////
/// Import FBXSDK
////////////////////////////////////////////////////////////////////////////////
bool model::import_fbxsdk( path path )
{
	BOOST_LOG_TRIVIAL(info) << "FBXSDK is importing " << path;

	auto manager = make_fbx<FbxManager>();

	auto io_settings = make_fbx<FbxIOSettings>(manager.get(), "io_settings");
	manager->SetIOSettings(io_settings.get());

	auto importer = make_fbx<FbxImporter>(manager.get(), "importer");

	if (!importer->Initialize(path.string().c_str(), -1, manager->GetIOSettings()))
	{
		BOOST_LOG_TRIVIAL(warning) << importer->GetStatus().GetErrorString();
		return false;
	}

	auto scene = make_fbx<FbxScene>(manager.get(), "scene");
	importer->Import(scene.get());

	auto converter = FbxGeometryConverter{manager.get()};
	converter.SplitMeshesPerMaterial(scene.get(), true);
	//converter.Triangulate(scene.get(), true);

	std::queue<FbxNode*> nodes;
	nodes.push(scene->GetRootNode());

	mesh::vector_container vertices, normals, texture_coordinates;
	mesh::index_container indices;
	while (!nodes.empty())
	{
		const auto node = nodes.front();
		nodes.pop();

		for (int c = 0; node->GetChildCount() > c; ++c) 
			nodes.push(node->GetChild(c));

		for (int a = 0; node->GetNodeAttributeCount() > a; ++a)
		{
			const auto attribute = node->GetNodeAttributeByIndex(a);		
			switch (attribute->GetAttributeType())
			{
			case FbxNodeAttribute::eMesh:
			{
				if (!parse_mesh(*this, static_cast<const FbxMesh*>(attribute), path, vertices, normals, texture_coordinates, indices))
					return false;
			} break;

			case FbxNodeAttribute::eLight:
				if (!parse_light(*this, static_cast<const FbxLight*>(attribute), node))
					return false;
				break;

			case FbxNodeAttribute::eNull:
				break;

			default:
				{
					BOOST_LOG_TRIVIAL(info) << "Unparsed FBX attribute on node: \"" << node->GetName() << "\"";
				}
				break;
			}
		}
	}

	BOOST_LOG_TRIVIAL(info) << "Imported model " << path;
	return true;
}
    
#endif // #ifndef NO_FBX



////////////////////////////////////////////////////////////////////////////////
/// Import Assimp
////////////////////////////////////////////////////////////////////////////////
void parse_mesh( 
	model& model, 
	const aiScene* const scene,
	const aiMesh* const ai_mesh,
	path path, 
	const aiMatrix4x4 transformation )
{
	aiMatrix3x3 transformation3x3{transformation};

	// Vertices
	auto ai_vertices_begin = ai_mesh->mVertices;
	auto ai_vertices_end = ai_vertices_begin + ai_mesh->mNumVertices;
	transform(ai_vertices_begin, ai_vertices_end, ai_vertices_begin, 
		[transformation]( auto vec3 ){ return transformation * vec3; });
	mesh::vector_container vertices{reinterpret_cast<float*>(ai_vertices_begin), reinterpret_cast<float*>(ai_vertices_end)};

	// Normals
	auto ai_normals_begin = ai_mesh->mNormals;
	auto ai_normals_end = ai_normals_begin + ai_mesh->mNumVertices;
	transform(ai_normals_begin, ai_normals_end, ai_normals_begin, 
		[transformation3x3]( auto vec3 ){ return transformation3x3 * vec3; });
	mesh::vector_container normals{reinterpret_cast<float*>(ai_normals_begin), reinterpret_cast<float*>(ai_normals_end)};

	// Texture coordinates
	auto ai_texture_coordinates_begin = ai_mesh->mTextureCoords[0];
	auto ai_texture_coordinates_end = ai_texture_coordinates_begin + ai_mesh->mNumVertices;
	mesh::vector_container texture_coordinates;
	texture_coordinates.reserve(2 * ai_mesh->mNumVertices);
	auto texture_coordinates_begin = elementwise<2>(back_inserter(texture_coordinates));
	if (ai_mesh->HasTextureCoords(0))
		copy(ai_texture_coordinates_begin, ai_texture_coordinates_end, texture_coordinates_begin);

	// Indices
	const static int indices_per_face{3};
	auto ai_faces_begin = ai_mesh->mFaces;
	auto ai_faces_end = ai_mesh->mFaces + ai_mesh->mNumFaces;
	mesh::index_container indices;
	indices.reserve(ai_mesh->mNumFaces * indices_per_face);
	for_each(ai_faces_begin, ai_faces_end, [&]( aiFace face ) {
		// Assimp's triangulation should ensure the following.
		assert(indices_per_face == face.mNumIndices);

		indices.insert(indices.end(), face.mIndices, face.mIndices + face.mNumIndices);
	});

	// Material
	aiMaterial* ai_material = scene->mMaterials[ai_mesh->mMaterialIndex];

	using namespace argument;
	aiColor3D color;
	ai_material->Get(AI_MATKEY_COLOR_AMBIENT, color);
	material material{ambient{color.r, color.g, color.b}};

	ai_material->Get(AI_MATKEY_COLOR_DIFFUSE, color);
	material.set(diffuse{color.r, color.g, color.b});

	ai_material->Get(AI_MATKEY_COLOR_SPECULAR, color);
	material.set(specular{color.r, color.g, color.b});

	ai_material->Get(AI_MATKEY_COLOR_EMISSIVE, color);
	material.set(emissive{color.r, color.g, color.b});

	ai_material->Get(AI_MATKEY_OPACITY, material.alpha);
	ai_material->Get(AI_MATKEY_SHININESS, material.shininess);

	aiString texture_string;
	if (AI_SUCCESS == ai_material->Get(AI_MATKEY_TEXTURE(aiTextureType_AMBIENT, 0), texture_string))
		material.ambient_texture = path.parent_path() / std::string(texture_string.data, &texture_string.data[texture_string.length]);
	if (AI_SUCCESS == ai_material->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), texture_string))
		material.diffuse_texture = path.parent_path() / std::string(texture_string.data, &texture_string.data[texture_string.length]);
	if (AI_SUCCESS == ai_material->Get(AI_MATKEY_TEXTURE(aiTextureType_SPECULAR, 0), texture_string))
		material.specular_texture = path.parent_path() / std::string(texture_string.data, &texture_string.data[texture_string.length]);
	if (AI_SUCCESS == ai_material->Get(AI_MATKEY_TEXTURE(aiTextureType_HEIGHT, 0), texture_string))
		material.height_texture = path.parent_path() / std::string(texture_string.data, &texture_string.data[texture_string.length]);

	model.meshes.emplace_back(
		std::move(material),
		GL_TRIANGLES,
		std::move(vertices),
		std::move(normals),
		std::move(texture_coordinates),
		std::move(indices));
}




aiMatrix4x4 get_global_transformation( const aiNode* node ) 
{
	auto transformation = node->mTransformation;
	while (node->mParent)
	{
		auto parent = node->mParent;
		transformation = parent->mTransformation * transformation;
		node = parent;
	}
	return transformation;
}



bool model::import_assimp( path path )
{
	BOOST_LOG_TRIVIAL(info) << "Assimp is importing " << path;

	Assimp::Importer importer;
	importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, aiComponent_COLORS);
	importer.SetPropertyInteger(AI_CONFIG_PP_SLM_VERTEX_LIMIT, 1024 * 1024);
	importer.SetPropertyInteger(AI_CONFIG_PP_SLM_TRIANGLE_LIMIT, 1024 * 1024 / 3);
	importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_POINT | aiPrimitiveType_LINE);
	const aiScene* scene = importer.ReadFile(
		path.string().c_str(),
		aiProcess_CalcTangentSpace		| 
		aiProcess_JoinIdenticalVertices	|
		aiProcess_Triangulate			|
		aiProcess_RemoveComponent		|
		aiProcess_GenNormals			|
		aiProcess_SplitLargeMeshes		|
		aiProcess_ValidateDataStructure	|
		aiProcess_ImproveCacheLocality	|
		aiProcess_SortByPType			| // Configured to remove points and lines
		aiProcess_FindInvalidData		|
		aiProcess_GenUVCoords			|
		aiProcess_TransformUVCoords		|
		aiProcess_OptimizeMeshes		);

	// If Assimp fails, we give up.
	if (!scene)	
	{
		BOOST_LOG_TRIVIAL(warning) << importer.GetErrorString();
		return false;
	}

	// Parse the scene hierarchy
	std::queue<pair<aiNode*, aiMatrix4x4>> nodes;
	nodes.emplace(scene->mRootNode, scene->mRootNode->mTransformation);

	while (!nodes.empty())
	{
		const auto pair = nodes.front();
		const auto node = pair.first;
		const auto transformation = pair.second * node->mTransformation;
		nodes.pop();
		
		for (unsigned int c{0}; node->mNumChildren > c; ++c) 
			nodes.emplace(node->mChildren[c], transformation);
		
		for (unsigned int m{0}; node->mNumMeshes > m; ++m)
			parse_mesh(*this, scene, scene->mMeshes[node->mMeshes[m]], path, transformation);
	}

	// Parse the lights
	for (unsigned int l = 0; scene->mNumLights > l; ++l)
	{
		const auto ai_light = scene->mLights[l];
		const auto node = scene->mRootNode->FindNode(ai_light->mName);
		auto transformation = get_global_transformation(node);

		const auto meta_data = node->mMetaData;
		if (meta_data)
		{
			int user_properties;
			if (meta_data->Get("radius", user_properties))
			{
				int breakpoint = 2;
				breakpoint = 3;
			}

		}

		auto ai_light_position = transformation * ai_light->mPosition;
		auto ai_light_direction = (ai_light->mDirection.SquareLength() > 0.0f)
			? aiMatrix3x3{transformation} * ai_light->mDirection
			: aiMatrix3x3{transformation} * aiVector3D{0.0, 1.0, 0.0};
		
		glm::vec3 position{ai_light_position[0], ai_light_position[1], ai_light_position[2]};
		glm::vec3 direction{ai_light_direction[0], ai_light_direction[1], ai_light_direction[2]};
		glm::vec4 color{ai_light->mColorDiffuse[0], ai_light->mColorDiffuse[1], ai_light->mColorDiffuse[2], 1.0f};
		
		light_type type;
		switch (ai_light->mType)
		{
		case aiLightSource_POINT: type = light_type::point; break;
		case aiLightSource_DIRECTIONAL: type = light_type::directional; break;
		case aiLightSource_SPOT: type = light_type::spot; break;
		case aiLightSource_UNDEFINED:
			BOOST_LOG_TRIVIAL(info) << "Unsupported light type found: \"" << ai_light->mName.C_Str() << "\". Ignoring this light.";
			continue;
		default: assert(false); break;
		}

		lights.emplace_back(type, position, direction, color);
	}

	BOOST_LOG_TRIVIAL(info) << "Imported model " << path;
	return true;
}

#endif // #ifdef DEVELOPER_TOOLS



} // namespace cpu
} // namespace renderer
} // namespace black_label
