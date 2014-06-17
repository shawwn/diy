#ifndef DIY_SERIALIZATION_HPP
#define DIY_SERIALIZATION_HPP

#include <vector>
#include <map>

namespace diy
{
  struct BinaryBuffer
  {
                        BinaryBuffer(int position_ = 0):
                          position(position_)                       {}

    inline void         save_binary(const char* x, int count);
    inline void         load_binary(char* x, int count);

    void                clear()                                     { buffer.clear(); reset(); }
    void                reset()                                     { position = 0; }
    void                swap(BinaryBuffer& o)                       { std::swap(position, o.position); std::swap(buffer, o.buffer); }

    int                 position;
    std::vector<char>   buffer;
  };

  template<class T>
  struct Serialization
  {
    static void         save(BinaryBuffer& bb, const T& x)          { bb.save_binary((const char*)  &x, sizeof(T)); }
    static void         load(BinaryBuffer& bb, T& x)                { bb.load_binary((char*)        &x, sizeof(T)); }
  };

  // Specialize these functions for types that need special handling
  template<class T>
  void                  save(BinaryBuffer& bb, const T& x)          { Serialization<T>::save(bb, x); }

  template<class T>
  void                  load(BinaryBuffer& bb, T& x)                { Serialization<T>::load(bb, x); }

  // save/load for std::vector<U>
  template<class U>
  struct Serialization< std::vector<U> >
  {
    typedef             std::vector<U>          Vector;

    static void         save(BinaryBuffer& bb, const Vector& v)
    {
      unsigned s = v.size();
      diy::save(bb, s);
      for (unsigned i = 0; i < s; ++i)
        diy::save(bb, v[i]);
    }

    static void         load(BinaryBuffer& bb, Vector& v)
    {
      unsigned s;
      diy::load(bb, s);
      v.resize(s);
      for (unsigned i = 0; i < s; ++i)
        diy::load(bb, v[i]);
    }
  };

  // save/load for std::pair<X,Y>
  template<class X, class Y>
  struct Serialization< std::pair<X,Y> >
  {
    typedef             std::pair<X,Y>          Pair;

    static void         save(BinaryBuffer& bb, const Pair& p)
    {
      diy::save(bb, p.first);
      diy::save(bb, p.second);
    }

    static void         load(BinaryBuffer& bb, Pair& p)
    {
      diy::load(bb, p.first);
      diy::load(bb, p.second);
    }
  };

  // save/load for std::map<K,V>
  template<class K, class V>
  struct Serialization< std::map<K,V> >
  {
    typedef             std::map<K,V>           Map;

    static void         save(BinaryBuffer& bb, const Map& m)
    {
      unsigned s = m.size();
      diy::save(bb, s);
      for (typename std::map<K,V>::const_iterator it = m.begin(); it != m.end(); ++it)
        diy::save(bb, *it);
    }

    static void         load(BinaryBuffer& bb, Map& m)
    {
      unsigned s;
      diy::load(bb, s);
      std::pair<K,V> p;
      for (unsigned i = 0; i < s; ++i)
      {
        diy::load(bb, p);
        m.insert(p);
      }
    }
  };
}

void
diy::BinaryBuffer::
save_binary(const char* x, int count)
{
  if (position + count > buffer.size())
    buffer.resize(position + count);        // TODO: double-check that this does the right thing with capacity
  std::copy(x, x + count, &buffer[position]);
  position += count;
}

void
diy::BinaryBuffer::
load_binary(char* x, int count)
{
  std::copy(&buffer[position], &buffer[position + count], x);
  position += count;
}

#endif