#include <black_label/lsystem.hpp>

#include <algorithm>
#include <utility>
#include <stack>


#include <glm/gtx/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>



namespace black_label
{
namespace lsystem
{

int production::fit_score( 
	const symbol& s, 
	const symbol& left_context, 
	const symbol& right_context,
	const cube& guide ) const
{
	int score = 0;

	if (s != predecessor)
		return 0;
	else
		++score;

	if (this->right_context)
	{
		if (right_context != this->right_context)
			return 0;
		else
			++score;
	}

	if (this->left_context)
	{
		if (left_context != this->left_context)
			return 0;
		else
			++score;
	}

	if (!condition(s, left_context, right_context, guide))
		return 0;

	return score;
}



word lsystem::find_successor( 
	const symbol& s, 
	const symbol& left_context, 
	const symbol& right_context,
	const cube& guide )
{
	auto applicable_production = std::max_element(
		productions.begin(),
		productions.end(), 
		[&s, &right_context, &left_context, &guide] ( 
			const production& p1, 
			const production& p2 )
		{ 
			return p1.fit_score(s, left_context, right_context, guide)
				< p2.fit_score(s, left_context, right_context, guide);
		});

	if (productions.end() == applicable_production || 0 == applicable_production->fit_score(s, left_context, right_context, guide))
		return s;

	return (*applicable_production)(s, left_context, right_context, guide);
}

template<typename InputIterator>
typename InputIterator::value_type lsystem::find_right_context( 
	InputIterator first,
	InputIterator last )
{
	int level = 0;

	for (auto r = first; last != r; ++r)
	{
		if (level < 0) break;

		if ('[' == r->letter)
		{
			++level;
			continue;
		}
		else if (']' == r->letter)
		{
			--level;
			continue;
		}

		if (0 < level) continue;

		if (ignores.cend() != std::find(ignores.cbegin(), ignores.cend(), *r)) continue;

		return *r;
	}

	return ' ';
}

template<typename InputIterator>
typename InputIterator::value_type lsystem::find_left_context( 
	InputIterator first,
	InputIterator last )
{
	int level = 0;
	bool me = true;

	for (auto r = first; last != r; ++r)
	{
		if (']' == r->letter)
		{
			++level;
			continue;
		}
		else if ('[' == r->letter)
		{
			if (0 == level) me = false;
			--level;
			continue;
		}

		if (0 < level || (level == 0 && !me)) continue;

		if (ignores.cend() != std::find(ignores.cbegin(), ignores.cend(), *r)) continue;

		return *r;
	}

	return ' ';
}

void lsystem::apply_productions( unsigned int times )
{
	word new_result, successor;

	for ( unsigned int i = 0; i < times; ++i )
	{
		word& result = results.back();
		turtle_interpretation::resolve_queries(start_turtle, result);

		for (auto s = result.cbegin(); result.cend() != s; ++s)
		{
			symbol right_context = find_right_context(s+1, result.cend());
			symbol left_context = find_left_context(
				word::const_reverse_iterator(s), 
				result.crend());

			successor = find_successor(*s, left_context, right_context, cube_guide);
			new_result.insert(
				new_result.end(), 
				successor.begin(),
				successor.end());
		}

		results.push_back(new_result);
		new_result.clear();
	}
}



void turtle_interpretation::resolve_queries( const turtle& start_turtle, word& w )
{
	std::stack<turtle> states;
	states.push(turtle(start_turtle));
	
	static glm::vec3 tropism(0.0f, -1.0f, 0.0f);
	glm::vec3 heading;

	for (auto s = w.begin(); w.end() != s; ++s)
	{
		switch (s->letter)
		{
		case 'T':
			heading = glm::vec3(states.top().transformation[1]);
			states.top().transformation = glm::rotate(
				states.top().transformation, 
				s->parameters[0]*glm::length(glm::cross(heading, tropism))
				*180.0f/glm::pi<float>(),
				glm::vec3(1.0f, 0.0f, 0.0f));
			break;

		case 'F':
			states.top().transformation = glm::translate(states.top().transformation, glm::vec3(0.0f, s->parameters[0], 0.0f));
			break;

		case '[':
			states.push(states.top());
			break;
		case ']':
			states.pop();
			break;

		case '+':
			if (1 == s->parameter_count)
				states.top().transformation = glm::rotate(states.top().transformation, s->parameters[0], glm::vec3(0.0f, 0.0f, 1.0f));
			else
				states.top().transformation = glm::rotate(states.top().transformation, 22.5f, glm::vec3(0.0f, 0.0f, 1.0f));
			break;
		case '-':
			if (1 == s->parameter_count)
				states.top().transformation = glm::rotate(states.top().transformation, -s->parameters[0], glm::vec3(0.0f, 0.0f, 1.0f));
			else
				states.top().transformation = glm::rotate(states.top().transformation, -22.5f, glm::vec3(0.0f, 0.0f, 1.0f));
			break;

		case '&':
			states.top().transformation = glm::rotate(states.top().transformation, s->parameters[0], glm::vec3(1.0f, 0.0f, 0.0f));
			break;
		case '^':
			states.top().transformation = glm::rotate(states.top().transformation, -s->parameters[0], glm::vec3(1.0f, 0.0f, 0.0f));
			break;

		case '\\':
			states.top().transformation = glm::rotate(states.top().transformation, s->parameters[0], glm::vec3(0.0f, 1.0f, 0.0f));
			break;
		case '/':
			states.top().transformation = glm::rotate(states.top().transformation, -s->parameters[0], glm::vec3(0.0f, 1.0f, 0.0f));
			break;

		case '|':
			states.top().transformation = glm::rotate(states.top().transformation, 180.0f, glm::vec3(0.0f, 0.0f, 1.0f));
			break;

		case 'p':
			s->parameters[0] = states.top().transformation[3][0];
			s->parameters[1] = states.top().transformation[3][1];
			s->parameters[2] = states.top().transformation[3][2];
			s->mat = states.top().transformation;
			break;
		default:
			break;
		}
	}
}

void turtle_interpretation::interpret()
{
	for (auto w = lsystem->results.cbegin() + vertices.size(); lsystem->results.cend() != w; ++w)
	{
		vertices.push_back(vertex_container());
		vertex_container& vs = vertices.back();
		normals.push_back(normal_container());
		normal_container& ns = normals.back();

		std::stack<turtle> states;
		states.push(turtle(lsystem->start_turtle));
		bool cut = false;

		static glm::vec3 tropism(0.0f, -1.0f, 0.0f);
		glm::vec3 heading;

		for (auto s = w->cbegin(); w->cend() != s; ++s)
		{
			switch (s->letter)
			{
			case '%':
				states.top().cut = true;
				break;

			case 'T':
				heading = glm::vec3(states.top().transformation[1]);
				states.top().transformation = glm::rotate(
					states.top().transformation, 
					s->parameters[0]*glm::length(glm::cross(heading, tropism))
						*180.0f/glm::pi<float>(),
					glm::vec3(1.0f, 0.0f, 0.0f));
				break;

			case 'F':
				{
					static bool is_uninitialized = true;
					static std::vector<glm::vec3> cylinder_positions, cylinder_normals;
					if (is_uninitialized)
					{
						is_uninitialized = false;
						const int N = 16;
						float angle = 0.0f;
						float increment = 2.0f*glm::pi<float>()/N;

						for (int i = 0; N > i; ++i)
						{
							cylinder_positions.emplace_back(glm::vec3(cos(angle), 0.0f, sin(angle)));
							cylinder_positions.emplace_back(glm::vec3(cos(angle), 1.0f, sin(angle)));
							cylinder_positions.emplace_back(glm::vec3(cos(angle+increment), 1.0f, sin(angle+increment)));
							cylinder_positions.emplace_back(glm::vec3(cos(angle+increment), 0.0f, sin(angle+increment)));

							cylinder_normals.emplace_back(glm::vec3(cos(angle), 0.0f, sin(angle)));
							cylinder_normals.emplace_back(glm::vec3(cos(angle), 0.0f, sin(angle)));
							cylinder_normals.emplace_back(glm::vec3(cos(angle+increment), 0.0f, sin(angle+increment)));
							cylinder_normals.emplace_back(glm::vec3(cos(angle+increment), 0.0f, sin(angle+increment)));

							angle += increment;
						}
					}
					
					if (!states.top().cut)
					{
						auto scale_matrix = glm::mat4(1.0f);
						scale_matrix[0][0] = states.top().width;
						scale_matrix[1][1] = s->parameters[0];
						scale_matrix[2][2] = states.top().width;

						auto v = cylinder_positions.cbegin();
						auto n = cylinder_normals.cbegin();
						for (; cylinder_positions.cend() != v; ++v, ++n)
						{
							vs.push_back(glm::vec3(states.top().transformation * scale_matrix * glm::vec4(*v, 1.0)));
							ns.push_back(glm::normalize(glm::inverseTranspose(glm::mat3(states.top().transformation)*glm::mat3(scale_matrix)) * *n));
						}

						//vs.push_back(glm::vec3(states.top().transformation * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)));
						//vs.push_back(glm::vec3(states.top().transformation * glm::vec4(0.0f, s->parameters[0], 0.0f, 1.0f)));
						states.top().transformation = glm::translate(states.top().transformation, glm::vec3(0.0f, s->parameters[0], 0.0f));
					}
				}
				break;

			case '[':
				states.push(states.top());
				break;
			case ']':
				states.pop();
				break;

			case '+':
				if (1 == s->parameter_count)
					states.top().transformation = glm::rotate(states.top().transformation, s->parameters[0], glm::vec3(0.0f, 0.0f, 1.0f));
				else
					states.top().transformation = glm::rotate(states.top().transformation, 22.5f, glm::vec3(0.0f, 0.0f, 1.0f));
				break;
			case '-':
				if (1 == s->parameter_count)
					states.top().transformation = glm::rotate(states.top().transformation, -s->parameters[0], glm::vec3(0.0f, 0.0f, 1.0f));
				else
					states.top().transformation = glm::rotate(states.top().transformation, -22.5f, glm::vec3(0.0f, 0.0f, 1.0f));
				break;

			case '!':
				states.top().width = s->parameters[0];
				break;

			case '&':
				states.top().transformation = glm::rotate(states.top().transformation, s->parameters[0], glm::vec3(1.0f, 0.0f, 0.0f));
				break;
			case '^':
				states.top().transformation = glm::rotate(states.top().transformation, -s->parameters[0], glm::vec3(1.0f, 0.0f, 0.0f));
				break;

			case '\\':
				states.top().transformation = glm::rotate(states.top().transformation, s->parameters[0], glm::vec3(0.0f, 1.0f, 0.0f));
				break;
			case '/':
				states.top().transformation = glm::rotate(states.top().transformation, -s->parameters[0], glm::vec3(0.0f, 1.0f, 0.0f));
				break;

			case '|':
				states.top().transformation = glm::rotate(states.top().transformation, 180.0f, glm::vec3(0.0f, 0.0f, 1.0f));
				break;

			case 'L':
				break;
			default:
				break;
			}
		}
	}
}

} // namespace lsystem
} // namespace black_label





/*
	for (auto s = test_lsystem.result.begin(); test_lsystem.result.end() != s; ++s)
	{
		switch (s->letter)
		{


		case 'F':
			glLineWidth(line_width_stack.top()*0.1f);
			glBegin(GL_LINES);
			glColor3ub(139 * 3/4, 69 * 3/4, 19 * 3/4);
			glVertex3f(0.0f, 0.0f, 0.0f);
			glVertex3f(0.0f, s->parameters[0], 0.0f);
			glEnd();
			glTranslatef(0.0f, s->parameters[0], 0.0f);
			break;
		
		case '[':
			glPushMatrix();
			line_width_stack.push(line_width_stack.top());
			break;
		case ']':
			glPopMatrix();
			line_width_stack.pop();
			break;

		case '!':
			line_width_stack.top() = s->parameters[0];
			break;

		case 'L':
			glBegin(GL_TRIANGLES);
			glColor3f(0.1f, 0.8f, 0.0f);
			glVertex3f(0.0f, 0.0f, 0.0f);
			glVertex3f(leaf_size*0.2f, 0.0f, leaf_size);
			glVertex3f(-leaf_size*0.2f, 0.0f, leaf_size);
			glEnd();
			break;

		default:
			break;
		}
	}
	*/