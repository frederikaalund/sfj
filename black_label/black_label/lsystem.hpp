// Version 0.1
#ifndef BLACK_LABEL_LSYSTEM_HPP
#define BLACK_LABEL_LSYSTEM_HPP

#include <string>
#include <vector>
#include <array>
#include <functional>
#include <memory>
#include <type_traits>

#include <glm/glm.hpp>



namespace black_label
{
namespace lsystem
{

struct z_gradient_type {};
const z_gradient_type z_gradient;

class scalar_field
{
public:
	friend void swap( scalar_field& rhs, scalar_field& lhs )
	{
		using std::swap;
		swap(rhs.size, lhs.size);
		swap(rhs.dimensions, lhs.dimensions);
		swap(rhs.position, lhs.position);
		swap(rhs.data, lhs.data);
	}
	scalar_field() {}
	scalar_field( scalar_field&& other ) { swap(*this, other); }
	scalar_field( const scalar_field& other )
		: size(other.size)
		, dimensions(other.dimensions)
		, position(other.position)
		, data(new float[dimensions*dimensions*dimensions])
	{ 
		//std::copy(other.data.get(), &other.data[dimensions*dimensions*dimensions], data.get());
	}
	scalar_field( float size, int dimensions, glm::vec3 position ) 
		: size(size)
		, dimensions(dimensions)
		, position(position)
		, data(new float[dimensions*dimensions*dimensions])
	{}
	scalar_field( float size, int dimensions, glm::vec3 position, z_gradient_type )
		: size(size)
		, dimensions(dimensions)
		, position(position)
		, data(new float[dimensions*dimensions*dimensions])
	{
		for (int z = 0; dimensions > z; ++z)
			for (int y = 0; dimensions > y; ++y)
				for (int x = 0; dimensions > x; ++x)
					data[dimensions*dimensions*z + dimensions*y + x] = 
						static_cast<float>(z);
	}

	scalar_field operator=( scalar_field rhs ) { swap(*this, rhs); return *this; }

	float size;
	int dimensions;
	glm::vec3 position;
	std::unique_ptr<float[]> data;
protected:
private:
};

class cube
{
public:
	cube() : size(20.0f) {}
	cube( float size, glm::vec3 position ) : size(size), position(position) {}

	float size;
	glm::vec3 position;
protected:
private:
};

class turtle
{
public:
	turtle() : transformation(0.0f), width(1.0f), cut(false) {}
	turtle( const glm::mat4 transformation, float width = 1.0f, bool cut = false ) 
		: transformation(transformation), width(width), cut(cut)
	{}

	glm::mat4 transformation;
	float width;
	bool cut;
};



class symbol
{
public:
	symbol( char letter ) : letter(letter), parameter_count(0) {}
	symbol( char letter, int parameter_count ) 
		: letter(letter)
		, parameter_count(parameter_count)
	{}
	symbol( char letter, float p1 ) : letter(letter), parameter_count(1)
	{
		parameters[0] = p1;
	}
	symbol( char letter, float p1, float p2 ) : letter(letter), parameter_count(2)
	{
		parameters[0] = p1;
		parameters[1] = p2;
	}
	symbol( char letter, float p1, float p2, float p3 ) : letter(letter), parameter_count(3)
	{
		parameters[0] = p1;
		parameters[1] = p2;
		parameters[2] = p3;
	}

	operator bool() const
	{
		return ' ' != letter;
	}

	bool operator==( const symbol& rhs ) const
	{ 
		return letter == rhs.letter && parameter_count == rhs.parameter_count;
	}
	bool operator!=( const symbol& rhs ) const
	{ 
		return !(rhs == *this);
	}

	char letter;
	std::array<float, 3> parameters;
	glm::mat4 mat;
	int parameter_count;
};



class word : public std::vector<symbol>
{
public:
	word( ) {}
	word( word&& other ) : std::vector<symbol>(std::move(other)) {}
	word( symbol* symbols, std::vector<symbol>::size_type size ) 
		: std::vector<symbol>(symbols, symbols+size ) {}
	word( const symbol& s ) { push_back(s); }
	word( const char* cstring ) 
	{
		std::string string = cstring;
		insert(end(), string.cbegin(), string.cend());
	}

	word& operator()( const char* cstring )
	{
		std::string string = cstring;
		insert(end(), string.cbegin(), string.cend());
		return *this;
	}
	word& operator()( const symbol&& s )
	{
		push_back(s);
		return *this;
	}
};



class production
{
public:
	template<typename F1>
	production( 
		const symbol& predecessor, 
		const F1& f,
		const symbol& left_context = ' ',
		const symbol& right_context = ' ' )
		: predecessor(predecessor)
		, left_context(left_context)
		, right_context(right_context)
		, f(f)
		, condition([] ( 
			const symbol& s, 
			const symbol& left_context, 
			const symbol& right_context,
			const cube& cube_guide ) -> bool { return true; })
	{}
	template<typename F1, typename F2>
	production( 
		const symbol& predecessor, 
		const F1& f, 
		const F2& condition,
		const symbol& left_context = ' ',
		const symbol& right_context = ' ',
		typename std::enable_if<!std::is_convertible<F2, symbol>::value>::type* 
			dummy1 = nullptr )
		: predecessor(predecessor)
		, left_context(left_context)
		, right_context(right_context)
		, f(f)
		, condition(condition) 
	{}

	int fit_score( 
		const symbol& s,
		const symbol& left_context,
		const symbol& right_context,
		const cube& guide ) const;

	word operator()( 
		const symbol& s,
		const symbol& left_context, 
		const symbol& right_context,
		const cube& guide ) const
	{ return f(s, left_context, right_context, guide); }

	symbol 
		predecessor,
		left_context,
		right_context;
	std::function<word ( 
		const symbol& s,
		const symbol& left_context,
		const symbol& right_context,
		const cube& cube_guide )> f;
	std::function<bool ( 
		const symbol& s, 
		const symbol& left_context,
		const symbol& right_context,
		const cube& cube_guide )> condition;
};



class lsystem
{
public:
	typedef std::vector<production> production_container;
	typedef std::vector<symbol> ignore_container;

	lsystem() {}

	lsystem( 
		const word& axiom, 
		const production_container& productions,
		const ignore_container& ignores = ignore_container() )
		: axiom(axiom)
		, productions(productions) 
		, ignores(ignores)
	{ results.push_back(axiom); }

	lsystem( const lsystem& other, cube cube_guide, const turtle& start_turtle )
		: axiom(other.axiom)
		, results(other.results)
		, productions(other.productions)
		, ignores(other.ignores)
		, cube_guide(cube_guide)
		, start_turtle(start_turtle)
	{}

	lsystem( const lsystem& other, scalar_field scalar_field_guide, const turtle& start_turtle )
		: axiom(other.axiom)
		, results(other.results)
		, productions(other.productions)
		, ignores(other.ignores)
		, scalar_field_guide(scalar_field_guide)
		, start_turtle(start_turtle)
	{}

	void apply_productions( unsigned int times );

	word axiom;
	std::vector<word> results;
	production_container productions;
	ignore_container ignores;
	cube cube_guide;
	scalar_field scalar_field_guide;
	turtle start_turtle;

private:
	word lsystem::find_successor( 
		const symbol& s, 
		const symbol& left_context, 
		const symbol& right_context,
		const cube& guide );

	template<typename InputIterator>
	typename InputIterator::value_type find_right_context( 
		InputIterator first,
		InputIterator last );
	template<typename InputIterator>
	typename InputIterator::value_type find_left_context( 
		InputIterator first,
		InputIterator last );
};



class turtle_interpretation
{
public:
	typedef std::vector<glm::vec3> vertex_container, normal_container;

	turtle_interpretation() : lsystem(nullptr), current_rewrite_count(0) {}

	turtle_interpretation( const turtle_interpretation& other ) 
		: lsystem(other.lsystem)
		, current_rewrite_count(other.current_rewrite_count)
		, vertices(other.vertices)
		, normals(other.normals) {}

	turtle_interpretation( const black_label::lsystem::lsystem* lsystem )
		: lsystem(lsystem), current_rewrite_count(0) {}

	static void resolve_queries( const turtle& start_turtle, word& w );
	void interpret();

	const lsystem* lsystem;
	int current_rewrite_count;
	std::vector<vertex_container> vertices;
	std::vector<normal_container> normals;
};

} // namespace lsystem
} // namespace black_label



#endif