#ifndef DIY_COVER_HPP
#define DIY_COVER_HPP

#include <vector>
#include <map>
#include <algorithm>

#include "types.hpp"
#include "serialization.hpp"
#include "assigner.hpp"

#include "factory.hpp"

namespace diy
{
  // Local view of a distributed representation of a cover, a completely unstructured link
  class Link: public Factory<Link>
  {
    public:
      using Neighbors = std::vector<BlockID>;

                Link(Key)                           {}  // for Factory
                Link()                              = default;
                Link(const Link&)                   = default;
                Link(Link&&)                        = default;
      Link&     operator=(const Link&)              = default;
      Link&     operator=(Link&&)                   = default;
      virtual   ~Link()                             {}  // need to be able to delete derived classes

      int       size() const                        { return static_cast<int>(neighbors_.size()); }
      inline
      int       size_unique() const;
      BlockID   target(int i) const                 { return neighbors_[static_cast<size_t>(i)]; }
      BlockID&  target(int i)                       { return neighbors_[static_cast<size_t>(i)]; }
      inline
      int       find(int gid) const;

      void      add_neighbor(const BlockID& block)  { neighbors_.push_back(block); }

      void      fix(const Assigner& assigner)       { for (unsigned i = 0; i < neighbors_.size(); ++i) { neighbors_[i].proc = assigner.rank(neighbors_[i].gid); } }

      void      swap(Link& other)                   { neighbors_.swap(other.neighbors_); }

      const Neighbors&
                neighbors() const                   { return neighbors_; }
      Neighbors&
                neighbors()                         { return neighbors_; }

      virtual void  save(BinaryBuffer& bb) const    { diy::save(bb, neighbors_); }
      virtual void  load(BinaryBuffer& bb)          { diy::load(bb, neighbors_); }

    private:
      Neighbors neighbors_;
  };

  template<class Bounds_>
  class RegularLink;

  using RegularGridLink         = RegularLink<DiscreteBounds>;
  using RegularContinuousLink   = RegularLink<ContinuousBounds>;

  // for a regular decomposition, it makes sense to address the neighbors by direction
  // and store local and neighbor bounds
  template<class Bounds_>
  class RegularLink: public Link::Registrar<RegularLink<Bounds_>>
  {
    public:
      typedef   Bounds_                             Bounds;

      typedef   std::map<Direction, int>            DirMap;
      typedef   std::vector<Direction>              DirVec;

    public:
                RegularLink():
                  dim_(0), core_(0), bounds_(0)               {}        // for Factory
                RegularLink(int dim, const Bounds& core__, const Bounds& bounds__):
                  dim_(dim), core_(core__), bounds_(bounds__) {}

      // dimension
      int       dimension() const                       { return dim_; }

      // direction
      int       direction(Direction dir) const;         // convert direction to a neighbor (-1 if no neighbor)
      Direction direction(int i) const                  { return dir_vec_[i]; }
      void      add_direction(Direction dir)            { int c = dir_map_.size(); dir_map_[dir] = c; dir_vec_.push_back(dir); }

      // wrap
      void       add_wrap(Direction dir)                { wrap_.push_back(dir); }
      Direction  wrap(int i) const                      { return wrap_[i]; }
      Direction& wrap(int i)                            { return wrap_[i]; }

      // bounds
      const Bounds& core() const                        { return core_; }
      Bounds&       core()                              { return core_; }
      const Bounds& bounds() const                      { return bounds_; }
      Bounds&       bounds()                            { return bounds_; }
      const Bounds& bounds(int i) const                 { return nbr_bounds_[i]; }
      void          add_bounds(const Bounds& bounds__)  { nbr_bounds_.push_back(bounds__); }

      void      swap(RegularLink& other)                { Link::swap(other); dir_map_.swap(other.dir_map_); dir_vec_.swap(other.dir_vec_); nbr_bounds_.swap(other.nbr_bounds_); std::swap(dim_, other.dim_); wrap_.swap(other.wrap_); std::swap(core_, other.core_); std::swap(bounds_, other.bounds_); }

      void      save(BinaryBuffer& bb) const
      {
          Link::save(bb);
          diy::save(bb, dim_);
          diy::save(bb, dir_map_);
          diy::save(bb, dir_vec_);
          diy::save(bb, core_);
          diy::save(bb, bounds_);
          diy::save(bb, nbr_bounds_);
          diy::save(bb, wrap_);
      }

      void      load(BinaryBuffer& bb)
      {
          Link::load(bb);
          diy::load(bb, dim_);
          diy::load(bb, dir_map_);
          diy::load(bb, dir_vec_);
          diy::load(bb, core_);
          diy::load(bb, bounds_);
          diy::load(bb, nbr_bounds_);
          diy::load(bb, wrap_);
      }

    private:
      int       dim_;

      DirMap    dir_map_;
      DirVec    dir_vec_;

      Bounds                    core_;
      Bounds                    bounds_;
      std::vector<Bounds>       nbr_bounds_;
      std::vector<Direction>    wrap_;
  };

  // Other cover candidates: KDTreeLink, AMRGridLink

  struct LinkFactory
  {
    public:
      static Link*          create(std::string name)
      {
          return Link::make(name);
      }

      inline static void    save(BinaryBuffer& bb, const Link* l);
      inline static Link*   load(BinaryBuffer& bb);
  };

  namespace detail
  {
      inline void instantiate_common_regular_links()
      {
          // Instantiate the common types to register them
          RegularLink<Bounds<int>>      rl_int;
          RegularLink<Bounds<float>>    rl_float;
          RegularLink<Bounds<double>>   rl_double;
          RegularLink<Bounds<long>>     rl_long;
      }
  }
}


void
diy::LinkFactory::
save(BinaryBuffer& bb, const Link* l)
{
    diy::save(bb, l->id());
    l->save(bb);
}

diy::Link*
diy::LinkFactory::
load(BinaryBuffer& bb)
{
    std::string id;
    diy::load(bb, id);
    Link* l = create(id);
    l->load(bb);
    return l;
}

int
diy::Link::
find(int gid) const
{
    for (int i = 0; i < size(); ++i)
  {
    if (target(i).gid == gid)
      return i;
  }
  return -1;
}
int
diy::Link::
size_unique() const
{
    std::vector<BlockID> tmp(neighbors_.begin(), neighbors_.end());
    std::sort(tmp.begin(), tmp.end());
    return static_cast<int>(std::unique(tmp.begin(), tmp.end()) - tmp.begin());
}

template<class Bounds>
int
diy::RegularLink<Bounds>::
direction(Direction dir) const
{
  DirMap::const_iterator it = dir_map_.find(dir);
  if (it == dir_map_.end())
    return -1;
  else
    return it->second;
}

#endif
