// Copyright (c) 2019 CNRS
// Authors: Joseph Mirabel
//
//
// This file is part of hpp-gui
// hpp-gui is free software: you can redistribute it
// and/or modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation, either version
// 3 of the License, or (at your option) any later version.
//
// hpp-gui is distributed in the hope that it will be
// useful, but WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Lesser Public License for more details.  You should have
// received a copy of the GNU Lesser General Public License along with
// hpp-gui  If not, see
// <http://www.gnu.org/licenses/>.

#ifndef HPP_GUI_HPP_FCL_PLUGIN_NODE_HH
#define HPP_GUI_HPP_FCL_PLUGIN_NODE_HH

#include <gepetto/viewer/node.h>

#include <hpp/fcl/BVH/BVH_model.h>

namespace hpp {
  namespace gui {
    typedef gepetto::viewer::Node Node;

    DEF_CLASS_SMART_PTR(BVHDisplay)

    class BVHDisplay : public Node
    {
      public:
        BVHDisplay (const std::string& filename, const std::string& name);

        void setLevel (const std::size_t level);

        const std::size_t& getLevel () const
        {
          return level_;
        }

        void setColor (const osgVector4& color);

        void init (hpp::fcl::SplitMethodType splitMethod);

      private:
        typedef hpp::fcl::OBB BoundingVolume;
        typedef hpp::fcl::BVHModel<BoundingVolume> BVH_t;
        typedef boost::shared_ptr < BVH_t > BVHPtr_t;

        void recursiveBuildTree (const BVH_t& bvh, int ibv, std::size_t level);

        struct BVLevel {
          //std::vector< ::osg::ShapeDrawableRefPtr > bvs;
          std::vector< ::osg::BoxRefPtr > boxes;
          ::osg::GeodeRefPtr geode;
        };
        const std::string filename_;
        std::vector <BVLevel> levels_;
        std::size_t level_;
    };
  } // namespace gui
} // namespace hpp

#endif // HPP_GUI_HPP_FCL_PLUGIN_NODE_HH
