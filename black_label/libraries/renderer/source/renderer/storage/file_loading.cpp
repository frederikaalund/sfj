#ifdef DEVELOPER_TOOLS

#define BLACK_LABEL_SeHARED_LIBRARY_EXPORT
#include <black_label/renderer/storage/file_loading.hpp>
#include <black_label/utility/log_severity_level.hpp>

#include <algorithm>
#include <queue>
#include <vector>

#include <boost/log/common.hpp>
#include <boost/log/sources/severity_logger.hpp>

#include <GL/glew.h>
#ifndef APPLE
#include <GL/gl.h>
#else
#include <OpenGL/gl.h>
#endif

#define FBXSDK_NEW_API
#include <fbxsdk.h>
#include <fbxsdk/fileio/fbxiosettings.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>



namespace black_label {
namespace renderer {
namespace storage {

using namespace utility;



////////////////////////////////////////////////////////////////////////////////
/// Scoped FBX Object
////////////////////////////////////////////////////////////////////////////////
template<typename T>
class scoped_fbx_object
{
public:
	scoped_fbx_object() : object(T::Create()) {}
	~scoped_fbx_object() { object->Destroy(); }

	operator T*() { return object; }
	T* operator ->() { return object; }

	T* object;
};



////////////////////////////////////////////////////////////////////////////////
/// Parse Functions
////////////////////////////////////////////////////////////////////////////////
void parse_mesh( 
	const FbxMesh* mesh, 
	std::vector<float>& vertices, 
	std::vector<float>& normals, 
	cpu::model& cpu_model, 
	gpu::model& gpu_model )
{
	vertices.clear(); normals.clear();
	auto control_points = mesh->GetControlPoints();
	auto control_point_count = mesh->GetControlPointsCount();
	for (int cp = 0; control_point_count > cp; ++cp)
	{
		vertices.push_back(static_cast<float>(control_points[cp][0]));
		vertices.push_back(static_cast<float>(control_points[cp][1]));
		vertices.push_back(static_cast<float>(control_points[cp][2]));

		for (int n = 0; mesh->GetElementNormalCount() > n; ++n)
		{
			auto normal = mesh->GetElementNormal(n);
			if (FbxGeometryElement::eByControlPoint == normal->GetMappingMode())
			{
				int id;

				switch (normal->GetReferenceMode())
				{
				case FbxGeometryElement::eDirect:
					id = cp;
					break;
				case FbxGeometryElement::eIndex:
					id = normal->GetIndexArray()[cp];
					break;
				}

				normals.push_back(static_cast<float>(normal->GetDirectArray()[id][0]));
				normals.push_back(static_cast<float>(normal->GetDirectArray()[id][1]));
				normals.push_back(static_cast<float>(normal->GetDirectArray()[id][2]));
			}
		}
	}



	// Assuming two's complement implementation of int
	// and that the result of the following reinterpret_cast is valid for
	// the current platform.
	unsigned int* indices = reinterpret_cast<unsigned int*>(mesh->GetPolygonVertices());
	int indices_count = 3 * mesh->GetPolygonCount();



	material material;
	material.diffuse = glm::vec3(1.0f);
	material.alpha = 1.0f;

	cpu::mesh cpu_mesh(
		std::move(material),
		GL_TRIANGLES,
		vertices.data(), 
		&vertices.back() + 1, 
		normals.data(), 
		nullptr, // TODO: Implement texture coordinate loading for FBX
		indices,
		&indices[indices_count]);

	gpu_model.push_back(gpu::mesh(cpu_mesh));
	cpu_model.push_back(std::move(cpu_mesh));
}



void parse_light(
	const FbxLight* fbx_light, 
	const FbxNode* node, 
	cpu::model& cpu_model, 
	gpu::model& gpu_model ) 
{
	auto fbx_color = fbx_light->Color.Get();
	auto color = glm::vec4(fbx_color[0], fbx_color[1], fbx_color[2], 1.0f);

	auto fbx_position = node->LclTranslation.Get();
	auto position = glm::vec3(fbx_position[0], fbx_position[1], fbx_position[2]);

	auto fbx_constant_attenuation = node->FindProperty("ConstantAttenuationFactor", true);
	auto constant_attenuation = (fbx_constant_attenuation.IsValid()) 
		? static_cast<FbxPropertyT<FbxFloat> >(fbx_constant_attenuation).Get()
		: 1.0f;

	auto fbx_linear_attenuation = node->FindProperty("LinearAttenuationFactor", true);
	auto linear_attenuation = (fbx_linear_attenuation.IsValid()) 
		? static_cast<FbxPropertyT<FbxFloat> >(fbx_linear_attenuation).Get()
		: 0.0f;

	auto fbx_quadratic_attenuation = node->FindProperty("QuadraticAttenuationFactor", true);
	auto quadratic_attenuation = (fbx_quadratic_attenuation.IsValid()) 
		? static_cast<FbxPropertyT<FbxFloat> >(fbx_quadratic_attenuation).Get()
		: 0.0f;


	auto light = black_label::renderer::light(position, color, constant_attenuation, linear_attenuation, quadratic_attenuation);

	gpu_model.add_light(light);
	cpu_model.add_light(light);
}



////////////////////////////////////////////////////////////////////////////////
/// Load FBX
////////////////////////////////////////////////////////////////////////////////
bool load_fbx(
	const boost::filesystem::path& path,
	cpu::model& cpu_model,
	gpu::model& gpu_model)
{
	boost::log::sources::severity_logger<utility::severity_level> log;
	scoped_fbx_object<FbxManager> fbx_manager;

	auto io_settings = FbxIOSettings::Create(fbx_manager, "io_settings");
	fbx_manager->SetIOSettings(io_settings);

	auto importer = FbxImporter::Create(fbx_manager, "importer");

	if (!importer->Initialize(path.string().c_str(), -1, fbx_manager->GetIOSettings()))
	{
		BOOST_LOG_SEV(log, warning) << importer->GetLastErrorString();
		return false;
	}

	auto scene = FbxScene::Create(fbx_manager, "scene");
	importer->Import(scene);

	std::queue<FbxNode*> nodes;
	nodes.push(scene->GetRootNode());

	std::vector<float> vertices, normals;
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
				parse_mesh(static_cast<const FbxMesh*>(attribute), vertices, normals, cpu_model, gpu_model);
				break;

			case FbxNodeAttribute::eLight:
				parse_light(static_cast<const FbxLight*>(attribute), node, cpu_model, gpu_model);
				break;

			case FbxNodeAttribute::eNull:
				break;

			default:
				{
					BOOST_LOG_SEV(log, info) << "Unparsed FBX attribute on node: \"" << node->GetName() << "\"";
				}
				break;
			}
		}
	}

	return true;
}



////////////////////////////////////////////////////////////////////////////////
/// Load Assimp
////////////////////////////////////////////////////////////////////////////////
bool load_assimp( 
	const boost::filesystem::path& path, 
	cpu::model& cpu_model, 
	gpu::model& gpu_model )
{
	boost::log::sources::severity_logger<utility::severity_level> log;
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(
		path.string().c_str(),
		aiProcess_CalcTangentSpace       | 
		aiProcess_GenNormals			 |
		aiProcess_GenUVCoords			 |
		aiProcess_Triangulate            |
		aiProcess_JoinIdenticalVertices  |
		aiProcess_PreTransformVertices   |
		aiProcess_SortByPType);

	// If Assimp fails, we give up.
	if (!scene)	
	{
		BOOST_LOG_SEV(log, warning) << importer.GetErrorString();
		return false;
	}

	std::vector<unsigned int> indices;
	std::vector<float> texture_coordinates;
	for (unsigned int m = 0; scene->mNumMeshes > m; ++m)
	{
		aiMesh* ai_mesh = scene->mMeshes[m];
		auto ai_vertices = ai_mesh->mVertices;
		auto ai_normals = ai_mesh->mNormals;
		auto ai_texture_coordinates = ai_mesh->mTextureCoords[0];
		auto ai_faces = ai_mesh->mFaces;

		texture_coordinates.clear();
		if (ai_mesh->HasTextureCoords(0))
			std::for_each(ai_texture_coordinates, &ai_texture_coordinates[ai_mesh->mNumVertices], 
				[&] ( aiVector3D& tc ) {
					texture_coordinates.push_back(tc[0]);
					texture_coordinates.push_back(tc[1]);
			});

		const static int indices_per_face = 3;
		indices.clear();
		indices.reserve(ai_mesh->mNumFaces * indices_per_face);
		for (auto f = ai_faces; &ai_faces[ai_mesh->mNumFaces] != f; ++f)
		{
			// Assimp's triangulation should ensure the following.
			assert(indices_per_face == f->mNumIndices);

			indices.insert(indices.end(), 
				f->mIndices, 
				&f->mIndices[f->mNumIndices]);
		}

		aiMaterial* ai_material = scene->mMaterials[ai_mesh->mMaterialIndex];
		material material;

		aiColor3D color;
		ai_material->Get(AI_MATKEY_COLOR_AMBIENT, color);
		material.ambient.r = color.r;
		material.ambient.g = color.g;
		material.ambient.b = color.b;

		ai_material->Get(AI_MATKEY_COLOR_DIFFUSE, color);
		material.diffuse.r = color.r;
		material.diffuse.g = color.g;
		material.diffuse.b = color.b;

		ai_material->Get(AI_MATKEY_COLOR_SPECULAR, color);
		material.specular.r = color.r;
		material.specular.g = color.g;
		material.specular.b = color.b;

		ai_material->Get(AI_MATKEY_COLOR_EMISSIVE, color);
		material.emissive.r = color.r;
		material.emissive.g = color.g;
		material.emissive.b = color.b;

		ai_material->Get(AI_MATKEY_OPACITY, material.alpha);
		ai_material->Get(AI_MATKEY_SHININESS, material.shininess);

		aiString texture_string;
		if (AI_SUCCESS == ai_material->Get(AI_MATKEY_TEXTURE(aiTextureType_AMBIENT, 0), texture_string))
			material.ambient_texture = std::string(texture_string.data, &texture_string.data[texture_string.length]);
		if (AI_SUCCESS == ai_material->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), texture_string))
			material.diffuse_texture = std::string(texture_string.data, &texture_string.data[texture_string.length]);
		if (AI_SUCCESS == ai_material->Get(AI_MATKEY_TEXTURE(aiTextureType_SPECULAR, 0), texture_string))
			material.specular_texture = std::string(texture_string.data, &texture_string.data[texture_string.length]);
		if (AI_SUCCESS == ai_material->Get(AI_MATKEY_TEXTURE(aiTextureType_HEIGHT, 0), texture_string))
			material.height_texture = std::string(texture_string.data, &texture_string.data[texture_string.length]);

		cpu::mesh cpu_mesh(
			std::move(material),
			GL_TRIANGLES,
			reinterpret_cast<float*>(ai_vertices), 
			reinterpret_cast<float*>(&ai_vertices[ai_mesh->mNumVertices]), 
			reinterpret_cast<float*>(&ai_normals[0]), 
			(!texture_coordinates.empty()) ? texture_coordinates.data() : nullptr,
			indices.data(),
			&indices.back() + 1);

		gpu_model.push_back(gpu::mesh(cpu_mesh));
		cpu_model.push_back(std::move(cpu_mesh));
	}

	for (unsigned int l = 0; scene->mNumLights > l; ++l)
	{
		aiLight* ai_light = scene->mLights[l];

		auto position = glm::vec3(ai_light->mPosition[0], ai_light->mPosition[1], ai_light->mPosition[2]);
		auto color = glm::vec4(ai_light->mColorAmbient[0], ai_light->mColorAmbient[1], ai_light->mColorAmbient[2], 1.0f);
		auto light = black_label::renderer::light(position, color, ai_light->mAttenuationConstant, ai_light->mAttenuationLinear, ai_light->mAttenuationQuadratic);

		gpu_model.add_light(light);
		cpu_model.add_light(light);
	}

	return true;
}


} // namespace storage
} // namespace renderer
} // namespace black_label

#endif
