#pragma once

#include "skel.hpp"

namespace xs
{
namespace loaders
{
std::shared_ptr<skeleton> load_skeleton(std::string_view filename);

std::shared_ptr<skinned_mesh> load_skinned_mesh(std::string_view filename);

std::shared_ptr<skeletal_anim> load_skeletal_anim(std::string_view filename);
}

}