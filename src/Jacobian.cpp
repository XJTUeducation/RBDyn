// This file is part of RBDyn.
//
// RBDyn is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// RBDyn is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with RBDyn.  If not, see <http://www.gnu.org/licenses/>.

// associated header
#include "Jacobian.h"

// includes
// std
#include <algorithm>
#include <stdexcept>

// RBDyn
#include "MultiBodyConfig.h"

namespace rbd
{

Jacobian::Jacobian(const MultiBody& mb, int bodyId, const Eigen::Vector3d& point):
  jointsPath_(),
  point_(point),
  jac_()
{
  int index = mb.sBodyIndexById(bodyId);

	int dof = 0;
	while(index != -1)
	{
		jointsPath_.insert(jointsPath_.begin(), index);
		dof += mb.joint(index).dof();

		index = mb.parent(index);
	}

	jac_.resize(6, dof);
}

MultiBody Jacobian::subMultiBody(const MultiBody& mb) const
{
	std::vector<Body> bodies;
	std::vector<Joint> joints;

	std::vector<int> pred;
	std::vector<int> succ;
	std::vector<int> parent;
	std::vector<sva::PTransform> Xfrom;
	std::vector<sva::PTransform> Xto;

	for(std::size_t index = 0; index < jointsPath_.size(); ++index)
	{
		int i = jointsPath_[index];
		// body info
		bodies.push_back(mb.body(i));
		parent.push_back(index - 1);

		// joint info
		joints.push_back(mb.joint(i));
		succ.push_back(index);
		pred.push_back(index - 1);
		Xfrom.push_back(mb.transformFrom(i));
		Xto.push_back(mb.transformTo(i));
	}

	return MultiBody(std::move(bodies), std::move(joints),
					std::move(pred), std::move(succ), std::move(parent), std::move(Xfrom),
					std::move(Xto));
}

const Eigen::MatrixXd&
Jacobian::jacobian(const MultiBody& mb, const MultiBodyConfig& mbc)
{
	const std::vector<Joint>& joints = mb.joints();
	const std::vector<int>& succ = mb.successors();
	const std::vector<sva::PTransform>& Xt = mb.transformsTo();

	int curJ = 0;

	for(std::size_t index = 0; index < jointsPath_.size(); ++index)
	{
		int i = jointsPath_[index];

		const sva::PTransform& X_i = mbc.jointConfig[i];
		sva::PTransform X_0_i(mbc.bodyPosW[succ[i]].rotation());
		sva::PTransform X_i_N = mbc.bodyPosW[jointsPath_.back()]*
			mbc.bodyPosW[i].inv();

		jac_.block(0, curJ, 6, joints[i].dof()) =
			(X_0_i.inv()*X_i_N*Xt[i]*X_i).matrix()*joints[i].motionSubspace();

		curJ += joints[i].dof();
	}

	return jac_;
}

const Eigen::MatrixXd&
Jacobian::sJacobian(const MultiBody& mb, const MultiBodyConfig& mbc)
{
	checkMatchBodyPos(mb, mbc);
	checkMatchJointConf(mb, mbc);

	int m = *std::max_element(jointsPath_.begin(), jointsPath_.end());
	if(m >= static_cast<int>(mb.nrJoints()))
	{
		throw std::domain_error("jointsPath mismatch MultiBody");
	}

	return jacobian(mb, mbc);
}

MultiBody Jacobian::sSubMultiBody(const MultiBody& mb) const
{
	int m = *std::max_element(jointsPath_.begin(), jointsPath_.end());
	if(m >= static_cast<int>(mb.nrJoints()))
	{
		throw std::domain_error("jointsPath mismatch MultiBody");
	}

	return subMultiBody(mb);
}

} // namespace rbd

