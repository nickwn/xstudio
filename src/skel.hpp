#pragma once

#include <array>
#include <vector>
#include <string_view>
#include <chrono>
#include <optional>

#include "math/math.hpp"
#include "draw_item.hpp"

namespace xs
{

class skeleton
{
public:

	using euler_rot = std::array<float, 3>;

	struct box
	{
		box() : min(-.1f, -.1f, -.1f), max(.1f, .1f, .1f) {}
		box(const Eigen::Vector3f& min_, const Eigen::Vector3f& max_) : min(min_), max(max_) {}

		Eigen::Vector3f min;
		Eigen::Vector3f max;
	};

	struct dof
	{
		using dim_limit = std::array<float, 2>;
		dof() : rotxlimit({ -100000.f, 100000.f }), rotylimit({ -100000.f, 100000.f }), rotzlimit({ -100000.f, 100000.f }), cur({ 0.f , 0.f, 0.f }) {}
		dof(const dim_limit& rxl_, const dim_limit& ryl_, const dim_limit& rzl_, const euler_rot& cur_) : 
			rotxlimit(rxl_), rotylimit(ryl_), rotzlimit(rzl_), cur(cur_) {}

		dim_limit rotxlimit;
		dim_limit rotylimit;
		dim_limit rotzlimit;
		euler_rot cur; // TODO: remove, calculate from quat
	};

	skeleton(std::vector<box> bone_boxes, std::vector<dof> bone_dofs, std::vector<std::uint32_t> bone_parents,
		std::vector<mth::transform> lcl_bone_transforms, std::vector<std::string> bone_names);

	draw_item draw_item(rhi::device* device, rhi::buffer* d_mvp_buf);

	const std::vector<mth::transform>& evaluate_bones();

	const euler_rot& bone_rot(std::size_t idx) { return bone_dofs_[idx].cur; }
	void update_bone_rot(std::size_t idx, euler_rot val);

	std::vector<mth::transform>& bone_transforms() { return lcl_bone_transforms_; }
	const std::vector<mth::transform>& bone_transforms() const { return lcl_bone_transforms_; }

	const std::vector<std::string>& bone_names() const { return bone_names_; }

	std::size_t find_bone_idx(std::string_view name) const
	{ 
		auto itr = std::find(std::begin(bone_names_), std::end(bone_names_), name); 
		return itr == std::end(bone_names_) ? std::distance(std::begin(bone_names_), itr) : -1;
	}

	std::size_t num_bones() const { return num_bones_; }

private:
	std::vector<box> bone_boxes_;
	std::vector<dof> bone_dofs_;
	std::vector<std::uint32_t> bone_parents_;
	std::vector<mth::transform> lcl_bone_transforms_;
	std::vector<mth::transform> gbl_bone_transforms_;
	std::vector<std::string> bone_names_;
	std::size_t num_bones_;

	rhi::device::scoped_mmap<Eigen::Vector3f> d_vert_pos_view_;
	rhi::device::ptr<rhi::uniform_set> d_mvp_uniforms_;
	rhi::device::ptr<rhi::buffer> d_vert_pos_buf_;
};

struct alignas(2) skin_weight
{
	std::uint8_t joint;
	std::uint8_t weight;
};

using skin_weights = std::array<skin_weight, 4>;

class skinned_mesh
{
public:

	skinned_mesh(std::vector<Eigen::Vector3f> vert_pos_attribs, std::vector<Eigen::Vector3f> vert_norm_attribs,
		std::vector<skin_weights> vert_sw_attribs, std::vector<std::uint32_t> inds, std::vector<Eigen::Matrix4f> binv_joints);

	void evaluate(const std::vector<mth::transform>& gbl_bone_transforms);

	draw_item draw_item(rhi::device* device, rhi::buffer* d_mvp_buf);

private:

	std::vector<Eigen::Vector3f> vert_pos_attribs_;
	std::vector<Eigen::Vector3f> vert_norm_attribs_;
	std::vector<skin_weights> vert_sw_attribs_;
	std::vector<std::uint32_t> inds_;
	std::vector<Eigen::Matrix4f> binv_joints_;
	std::vector<Eigen::Matrix4f> w_binv_joints_;
	rhi::device::scoped_mmap<Eigen::Matrix4f> d_w_binv_joints_view_;

	rhi::device::ptr<rhi::uniform_set> d_mvp_uniforms_;
	rhi::device::ptr<rhi::buffer> d_w_binv_joints_buf_;
	rhi::device::ptr<rhi::buffer> d_vert_pos_buf_;
	rhi::device::ptr<rhi::buffer> d_vert_norm_buf_;
	rhi::device::ptr<rhi::buffer> d_vert_sw_buf_;
	rhi::device::ptr<rhi::buffer> d_inds_buf_;
};

class rig
{
public:
	rig(std::shared_ptr<skeleton> skeleton, std::shared_ptr<skinned_mesh> skinned_mesh);

	void evaluate() { skinned_mesh_->evaluate(skeleton_->evaluate_bones()); }

	std::shared_ptr<skeleton> skel() const { return skeleton_; }
	std::shared_ptr<skinned_mesh> skin() const { return skinned_mesh_; }

private:
	std::shared_ptr<skeleton> skeleton_;
	std::shared_ptr<skinned_mesh> skinned_mesh_;
};

// TODO: turns out 16 bit float isn't very accurate, change this possibly
class compressed_curve // more like semi compressed
{
public:
	compressed_curve() = default;

	enum class extrap_type : std::uint8_t
	{
		constant,
		linear,
		cycle,
		cycle_offset,
		bounce
	};

	enum class interp_type : std::uint8_t
	{
		flat, linear, smooth, fixed
	};

	struct key
	{
		float value, tangent_in, tangent_out, time;
		interp_type interp_in, interp_out;
	};
	compressed_curve(std::vector<key> keys, extrap_type extrap_in, extrap_type extrap_out);

	float evaluate(const float t) const;

private:
	struct alignas(8) curve_span
	{
		std::array<std::uint16_t, 4> basis;
	};

	std::vector<curve_span> spans_;
	std::vector<float> times_;
	float len_;

	// precomputed stuff
	std::vector<float> inv_t_diffs_;
	float first_val_;
	float last_val_; // for cycle with offset
	float tangent_in_;
	float tangent_out_;

	extrap_type extrap_in_;
	extrap_type extrap_out_;
};

class skeletal_anim
{
public:
	skeletal_anim(std::vector<compressed_curve> channels, float t_start, float t_end) :
		channels_(std::move(channels)), 
		t_start_(t_start), 
		t_end_(t_end)
	{}

	using pose_t = std::vector<Eigen::Quaternionf>;
	std::pair<Eigen::Vector3f, pose_t> evaluate(const float t) const; 

private:
	float t_start_, t_end_;
	compressed_curve root_channel_;
	std::vector<compressed_curve> channels_;
};

class player
{
public:
	player(std::shared_ptr<skeletal_anim> anim) :
		skeletal_anim_(anim),
		rigs_(),
		last_update_(),
		playing_(false),
		cur_time_(0.f)
	{}

	inline void add_rig(std::shared_ptr<rig> rig) { rigs_.push_back(rig); }
	inline void play() { playing_ = true; }

	void update(std::optional<float> delta_t = std::optional<float>());

private:
	using time_t = std::chrono::time_point<std::chrono::steady_clock>;

	std::shared_ptr<skeletal_anim> skeletal_anim_;
	std::vector<std::weak_ptr<rig>> rigs_;
	std::optional<time_t> last_update_;
	bool playing_;
	float cur_time_;
};

}
