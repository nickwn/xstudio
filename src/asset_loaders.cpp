#include "asset_loaders.hpp"

#include <fstream>
#include <cassert>
#include <optional>

#include "math/math.hpp"

namespace xs
{
namespace loaders
{


static inline bool is_id_char(const char c)
{
	return std::isalpha(c) || std::isdigit(c) || c == '_' || c == '.' || c == '-';
}

static inline bool eat_tok(std::istream& file, const char tok)
{
	while (std::isspace(file.peek())) file.get(); // eat spaces and newlines
	const char p = file.peek();
	return file.peek() == tok;
}

static inline std::string parse_string_tok(std::istream& file)
{
	while (std::isspace(file.peek())) file.get(); // eat spaces and newlines
	const char p = file.peek();

	std::string tok;
	while (is_id_char(file.peek()))
	{
		tok.push_back(static_cast<char>(file.get()));
	}

	return tok;
}

template<typename T> requires mth::is_vec3<T>
static inline std::istream& operator>> (std::istream& stream, T& val)
{
	stream >> val.x();
	stream >> val.y();
	stream >> val.z();

	return stream;
}

template<std::size_t N>
static inline std::istream& operator>> (std::istream& stream, std::array<float, N>& val)
{
	for (std::size_t i = 0; i < N; i++)
	{
		stream >> val[i];
	}

	return stream;
}

struct balljoint
{
	balljoint() : lcl_transform(mth::transform::identity), box(), dof(), parent_idx(), name() {}
	mth::transform lcl_transform;
	skeleton::box box;
	skeleton::dof dof;
	uint32_t parent_idx;
	std::string name;
};

static inline void parse_balljoint(std::istream& stream, std::vector<balljoint>& joints, std::uint32_t parent_idx)
{
	std::string name = parse_string_tok(stream);
	assert(eat_tok(stream, '{'));
	stream.get();

	std::uint32_t cur_idx = static_cast<std::uint32_t>(joints.size());
	joints.push_back(balljoint());

	balljoint cur_joint;
	cur_joint.name = std::move(name);
	cur_joint.parent_idx = parent_idx;

	std::optional<Eigen::Vector3f> offset;
	std::optional<skeleton::euler_rot> pose;
	while (!eat_tok(stream, '}'))
	{
		std::string joint_prop = parse_string_tok(stream);

		if (joint_prop == "offset")
		{
			offset = Eigen::Vector3f();
			stream >> *offset;
		}
		else if (joint_prop == "boxmin")
		{
			stream >> cur_joint.box.min;
		}
		else if (joint_prop == "boxmax")
		{
			stream >> cur_joint.box.max;
		}
		else if (joint_prop == "rotxlimit")
		{
			stream >> cur_joint.dof.rotxlimit;
		}
		else if (joint_prop == "rotylimit")
		{
			stream >> cur_joint.dof.rotylimit;
		}
		else if (joint_prop == "rotzlimit")
		{
			stream >> cur_joint.dof.rotzlimit;
		}
		else if (joint_prop == "pose")
		{
			pose = skeleton::euler_rot();
			stream >> *pose;
		}
		else if (joint_prop == "balljoint")
		{
			parse_balljoint(stream, joints, cur_idx);
		}
		else
		{
			assert(false && "invalid balljoint prop");
		}
	}

	assert(stream.get() == '}'); // eat }
	assert(offset); // offset is mandatory (i think?)

	if (!pose)
	{
		pose = { 0.f, 0.f, 0.f };
	}

	(*pose)[0] = std::clamp((*pose)[0], cur_joint.dof.rotxlimit[0], cur_joint.dof.rotxlimit[1]);
	(*pose)[1] = std::clamp((*pose)[1], cur_joint.dof.rotylimit[0], cur_joint.dof.rotylimit[1]);
	(*pose)[2] = std::clamp((*pose)[2], cur_joint.dof.rotzlimit[0], cur_joint.dof.rotzlimit[1]);

	cur_joint.dof.cur = *pose;

	cur_joint.lcl_transform = mth::transform(*offset, mth::to_quat((*pose)[0], (*pose)[1], (*pose)[2]));

	joints[cur_idx] = cur_joint;
}

std::shared_ptr<skeleton> load_skeleton(std::string_view filename)
{
	std::vector<balljoint> joints;

	std::ifstream file = std::ifstream(filename.data());

	assert(parse_string_tok(file) == "balljoint");
	parse_balljoint(file, joints, -1);

	file.close();

	std::vector<skeleton::box> bone_boxes;
	std::vector<skeleton::dof> bone_dofs;
	std::vector<std::uint32_t> bone_parents;
	std::vector<mth::transform> lcl_bone_transforms;
	std::vector<mth::transform> gbl_bone_transforms;
	std::vector<std::string> bone_names;

	bone_boxes.reserve(joints.size());
	bone_dofs.reserve(joints.size());
	bone_parents.reserve(joints.size());
	lcl_bone_transforms.reserve(joints.size());
	bone_names.reserve(joints.size());

	for (balljoint& joint : joints)
	{
		bone_boxes.push_back(joint.box);
		bone_dofs.push_back(joint.dof);
		bone_parents.push_back(joint.parent_idx);
		lcl_bone_transforms.push_back(joint.lcl_transform);
		bone_names.push_back(std::move(joint.name));
	}

	return std::make_shared<skeleton>(std::move(bone_boxes), std::move(bone_dofs), std::move(bone_parents), 
		std::move(lcl_bone_transforms), std::move(bone_names));
}

// --- skin file loading --- //

static inline std::uint8_t& num_skin_vert_attachments(skin_weights& weights)
{
	return weights[3].weight;
}

static inline std::istream& operator>> (std::istream& stream, skin_weights& val)
{
	std::size_t num_attachments;
	stream >> num_attachments;

	for (std::size_t i = 0; i < num_attachments; i++)
	{
		std::size_t joint_idx;
		stream >> joint_idx;
		val[i].joint = static_cast<std::uint8_t>(joint_idx);
		float weightf;
		stream >> weightf;
		val[i].weight = std::uint8_t(weightf * 255.f);
	}
	num_skin_vert_attachments(val) = static_cast<std::uint8_t>(num_attachments);

	return stream;
}

static inline std::istream& operator>> (std::istream& stream, Eigen::Matrix4f& val)
{
	std::string matrix_name = parse_string_tok(stream);
	assert(matrix_name == "matrix");

	assert(eat_tok(stream, '{')); stream.get();
	for (std::uint8_t r = 0; r < 4; r++)
	{
		for (std::uint8_t c = 0; c < 3; c++)
		{
			stream >> val(c, r);
		}
	}
	assert(eat_tok(stream, '}')); stream.get();

	val(3, 0) = 0.f;
	val(3, 1) = 0.f;
	val(3, 2) = 0.f;
	val(3, 3) = 1.f;

	return stream;
}

static inline std::istream& operator>> (std::istream& stream, compressed_curve::key& val)
{
	auto get_interp = [](const std::string_view interp_str) {
		if (interp_str == "flat")
		{
			return compressed_curve::interp_type::flat;
		}
		else if (interp_str == "linear")
		{
			return compressed_curve::interp_type::linear;
		}
		else if (interp_str == "smooth")
		{
			return compressed_curve::interp_type::smooth;
		}
		else
		{
			assert(false && "invalid interp");
		}
		return compressed_curve::interp_type::flat; // ugh
	};

	stream >> val.time;
	stream >> val.value;

	std::string try_in = parse_string_tok(stream);
	try
	{
		val.tangent_in = std::stof(try_in);
		val.interp_in = compressed_curve::interp_type::fixed;
	}
	catch (const std::invalid_argument& e)
	{
		val.interp_in = get_interp(try_in);
	}

	std::string try_out = parse_string_tok(stream);
	try
	{
		val.tangent_out = std::stof(try_out);
		val.interp_out = compressed_curve::interp_type::fixed;
	}
	catch (const std::invalid_argument& e)
	{
		val.interp_out = get_interp(try_out);
	}

	return stream;
}

template<typename T>
static inline std::istream& operator>> (std::istream& stream, std::vector<T>& val)
{
	for (T& elem : val)
	{
		stream >> elem;
	}

	return stream;
}

struct skin_file
{
	std::vector<Eigen::Vector3f> positions;
	std::vector<Eigen::Vector3f> normals;
	std::vector<skin_weights> skinweights;
	std::vector<std::uint32_t> triangles;
	std::vector<Eigen::Matrix4f> bindings;
};

template<typename T>
static inline void parse_vector(std::istream& stream, std::vector<T>& vec) 
{
	std::size_t size;
	stream >> size;

	using vec_value_type = typename std::decay_t<decltype(vec)>::value_type;
	if constexpr (std::same_as<vec_value_type, std::uint32_t>)
	{
		size *= 3;
	}

	vec.resize(size);
	assert(eat_tok(stream, '{')); stream.get();
	stream >> vec;
	assert(eat_tok(stream, '}')); stream.get();
};

static inline skin_file parse_skin(std::istream& stream)
{
	skin_file result;

	while (!eat_tok(stream, std::char_traits<char>::eof()))
	{
		std::string skin_prop = parse_string_tok(stream);

		if (skin_prop == "positions")
		{
			parse_vector(stream, result.positions);
		}
		else if (skin_prop == "normals")
		{
			parse_vector(stream, result.normals);
		}
		else if (skin_prop == "skinweights")
		{
			parse_vector(stream, result.skinweights);
		}
		else if (skin_prop == "triangles")
		{
			parse_vector(stream, result.triangles);
		}
		else if (skin_prop == "bindings")
		{
			parse_vector(stream, result.bindings);
		}
		else
		{
			assert(false && "invalid skin prop");
		}
	}

	return result;
}

std::shared_ptr<skinned_mesh> load_skinned_mesh(std::string_view filename)
{
	std::ifstream file = std::ifstream(filename.data());

	skin_file file_data = parse_skin(file);

	std::vector<Eigen::Matrix4f> binv_joints;
	binv_joints.resize(file_data.bindings.size());
	std::transform(std::begin(file_data.bindings), std::end(file_data.bindings), std::begin(binv_joints),
		[](const Eigen::Matrix4f& m) { return m.inverse(); }
	);

	return std::make_shared<skinned_mesh>(std::move(file_data.positions), std::move(file_data.normals), std::move(file_data.skinweights),
		std::move(file_data.triangles), std::move(binv_joints));
}

// --- animation file loading --- //
static inline std::istream& operator>> (std::istream& stream, compressed_curve::extrap_type& val)
{
	std::string extrap_str = parse_string_tok(stream);
	if (extrap_str == "constant")
	{
		val = compressed_curve::extrap_type::constant;
	}
	else if (extrap_str == "linear")
	{
		val = compressed_curve::extrap_type::linear;
	}
	else if (extrap_str == "cycle")
	{
		val = compressed_curve::extrap_type::cycle;
	}
	else if (extrap_str == "cycle_offset")
	{
		val = compressed_curve::extrap_type::cycle_offset;
	}
	else if (extrap_str == "bounce")
	{
		val = compressed_curve::extrap_type::bounce;
	}
	else
	{
		assert(false && "invalid extrap type");
	}

	return stream;
}

compressed_curve parse_channel(std::istream& stream)
{
	eat_tok(stream, '{');
	char aksk = stream.peek();
	assert(eat_tok(stream, '{')); stream.get();

	compressed_curve::extrap_type extrap_in = compressed_curve::extrap_type::constant;
	compressed_curve::extrap_type extrap_out = compressed_curve::extrap_type::constant;
	std::vector<compressed_curve::key> keys;

	while (!eat_tok(stream, '}'))
	{
		std::string channel_prop = parse_string_tok(stream);

		if (channel_prop == "extrapolate")
		{
			stream >> extrap_in;
			stream >> extrap_out;
		}
		else if (channel_prop == "keys")
		{
			parse_vector(stream, keys);
		}
	}

	assert(stream.get() == '}');

	return compressed_curve(std::move(keys), extrap_in, extrap_out);
}

std::shared_ptr<skeletal_anim> load_skeletal_anim(std::string_view filename)
{
	std::ifstream file = std::ifstream(filename.data());

	std::string anim_header_str = parse_string_tok(file);
	assert(anim_header_str == "animation");

	float t_start, t_end;
	std::vector<compressed_curve> channels;

	std::size_t used_idx = 0;
	assert(eat_tok(file, '{')); file.get();
	while (!eat_tok(file, '}'))
	{
		std::string anim_prop = parse_string_tok(file);

		if (anim_prop == "range")
		{
			file >> t_start;
			file >> t_end;
		}
		else if (anim_prop == "numchannels")
		{
			std::size_t num_channels;
			file >> num_channels;
			channels.resize(num_channels);
		}
		else if (anim_prop == "channel")
		{
			std::string channel_idx_str = parse_string_tok(file);
			std::size_t channel_idx = 0;
			try
			{
				channel_idx = std::stoull(channel_idx_str);
			}
			catch (const std::invalid_argument e) 
			{
				channel_idx = used_idx;
				used_idx++;
			}

			if (channel_idx >= channels.size())
			{
				channels.resize(channel_idx + 1);
			}

			compressed_curve temp_channel = parse_channel(file);
			channels[channel_idx] = std::move(temp_channel);
		}
		else
		{
			assert(false && "invalid anim prop");
		}
	}

	return std::make_shared<skeletal_anim>(std::move(channels), t_start, t_end);
}

}
}