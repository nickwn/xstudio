#include "math.hpp"

namespace xs
{
namespace mth
{

const transform transform::identity = transform(Eigen::Vector3f(0.f, 0.f, 0.f), Eigen::Quaternionf::Identity());

}
}