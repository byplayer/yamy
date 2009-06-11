//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// array.h


#ifndef _ARRAY_H
#  define _ARRAY_H

#  include <memory>


///
template <class T, class Allocator = std::allocator<T> >
class Array
{
public:
  typedef typename Allocator::reference         reference; ///
  typedef typename Allocator::const_reference   const_reference; ///
  typedef typename Allocator::pointer           iterator; ///
  typedef typename Allocator::const_pointer     const_iterator;	///
  typedef typename Allocator::size_type         size_type; ///
  typedef typename Allocator::difference_type   difference_type; ///
  typedef T                                     value_type;	///
  typedef Allocator                             allocator_type;	///
  typedef typename Allocator::pointer           pointer; ///
  typedef typename Allocator::const_pointer     const_pointer; ///
#if 0
  typedef std::reverse_iterator<iterator>       reverse_iterator; ///
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;	///
#endif	  
private:
  Allocator m_allocator;			/// 
  size_type m_size;				/// 
  pointer m_buf;				/// array buffer

public:
  /// constructor
  explicit Array(const Allocator& i_allocator = Allocator())
    : m_allocator(i_allocator), m_size(0), m_buf(NULL)  { }
	  
  /// constructor
  explicit Array(size_type i_size, const T& i_value = T(),
		 const Allocator& i_allocator = Allocator())
    : m_allocator(i_allocator), m_size(i_size),
      m_buf(m_allocator.allocate(m_size, 0))
  {
    std::uninitialized_fill_n(m_buf, i_size, i_value);
  }
	  
  /// constructor
  template <class InputIterator>
  Array(InputIterator i_begin, InputIterator i_end,
	const Allocator& i_allocator = Allocator())
    : m_allocator(i_allocator), m_size(distance(i_begin, i_end)),
      m_buf(Allocator::allocate(m_size, 0))
  {
    std::uninitialized_copy(i_begin, i_end, m_buf);
  }
	  
  /// copy constructor
  Array(const Array& i_o) : m_size(0), m_buf(NULL) { operator=(i_o); }

  /// destractor
  ~Array() { clear(); }

  ///
  Array& operator=(const Array& i_o)
  {
    if (&i_o != this)
    {
      clear();
      m_size = i_o.m_size;
      m_buf = m_allocator.allocate(m_size, 0);
      std::uninitialized_copy(i_o.m_buf, i_o.m_buf + m_size, m_buf);
    }
    return *this;
  }
  ///
  allocator_type get_allocator() const { return Allocator(); }
  /// return pointer to the array buffer
  typename allocator_type::pointer get() { return m_buf; }
  /// return pointer to the array buffer
  typename allocator_type::const_pointer get() const { return m_buf; }
  ///
  iterator begin() { return m_buf; }
  ///
  const_iterator begin() const { return m_buf; }
  ///
  iterator end() { return m_buf + m_size; }
  ///
  const_iterator end() const { return m_buf + m_size; }
#if 0
  ///
  reverse_iterator rbegin() { reverse_iterator(end()); }
  ///
  const_reverse_iterator rbegin() const { const_reverse_iterator(end()); }
  ///
  reverse_iterator rend() { reverse_iterator(begin()); }
  ///
  const_reverse_iterator rend() const { const_reverse_iterator(begin()); }
#endif
  ///
  size_type size() const { return m_size; }
  ///
  size_type max_size() const { return -1; }
  /// resize the array buffer.  NOTE: the original contents are cleared.
  void resize(size_type i_size, const T& i_value = T())
  {
    clear();
    m_size = i_size;
    m_buf = m_allocator.allocate(m_size, 0);
    std::uninitialized_fill_n(m_buf, i_size, i_value);
  }
  /// resize the array buffer.  
  template <class InputIterator>
  void resize(InputIterator i_begin, InputIterator i_end)
  {
    clear();
    m_size = distance(i_begin, i_end);
    m_buf = m_allocator.allocate(m_size, 0);
    std::uninitialized_copy(i_begin, i_end, m_buf);
  }
  /// expand the array buffer. the contents of it are copied to the new one
  void expand(size_type i_size, const T& i_value = T())
  {
    ASSERT( m_size <= i_size );
    if (!m_buf)
      resize(i_size, i_value);
    else
    {
      pointer buf = m_allocator.allocate(i_size, 0);
      std::uninitialized_copy(m_buf, m_buf + m_size, buf);
      std::uninitialized_fill_n(buf + m_size, i_size - m_size, i_value);
      clear();
      m_size = i_size;
      m_buf = buf;
    }
  }
  ///
  bool empty() const { return !m_buf; }
  ///
  reference operator[](size_type i_n) { return *(m_buf + i_n); }
  ///
  const_reference operator[](size_type i_n) const
  { return *(m_buf + i_n); }
  ///
  const_reference at(size_type i_n) const
  { return *(m_buf + i_n); }
  ///
  reference at(size_type i_n)
  { return *(m_buf + i_n); }
  ///
  reference front() { return *m_buf; }
  ///
  const_reference front() const { return *m_buf; }
  ///
  reference back() { return *(m_buf + m_size - 1); }
  ///
  const_reference back() const { return *(m_buf + m_size - 1); }
  ///
  void swap(Array &i_o)
  {
    if (&i_o != this)
    {
      pointer buf = m_buf;
      size_type size = m_size;
      m_buf = i_o.m_buf;
      m_size = i_o.m_size;
      i_o.m_buf = buf;
      i_o.m_size = size;
    }
  }
  ///
  void clear()
  {
    if (m_buf)
    {
      for (size_type i = 0; i < m_size; i ++)
	m_allocator.destroy(&m_buf[i]);
      m_allocator.deallocate(m_buf, m_size);
      m_buf = 0;
      m_size = 0;
    }
  }
};

#endif // _ARRAY_H
