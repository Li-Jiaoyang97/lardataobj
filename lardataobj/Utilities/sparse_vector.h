/**
 * @file    lardataobj/Utilities/sparse_vector.h
 * @brief   Class defining a sparse vector (holes are zeroes)
 * @author  Gianluca Petrillo (petrillo@fnal.gov)
 * @date    April 22, 2014
 * @version 1.0
 *
 */

#ifndef LARDATAOBJ_UTILITIES_SPARSE_VECTOR_H
#define LARDATAOBJ_UTILITIES_SPARSE_VECTOR_H

// C/C++ standard library
#include <algorithm> // std::upper_bound(), std::max()
#include <cstddef>   // std::ptrdiff_t
#include <iterator>  // std::distance()
#include <numeric>   // std::accumulate
#include <ostream>
#include <stdexcept>   // std::out_of_range()
#include <type_traits> // std::is_integral
#include <vector>

/// Namespace for generic LArSoft-related utilities.
namespace lar {

  // -----------------------------------------------------------------------------
  // ---  utility classes for sparse_vector
  // ---

  /**
 * @brief Little class storing a value.
 * @tparam T type of the stored value
 *
 * This class stores a constant value and returns it as conversion to type T.
 * It also acts as a left-value of type T, except that the assigned value
 * is ignored.
 */
  template <typename T>
  class const_value_box {
    typedef const_value_box<T> this_t;

  public:
    typedef T value_type; ///< type of the value stored

    /// Default constructor: stores default value
    const_value_box() {}

    /// Constructor: stores the specified value
    explicit const_value_box(value_type new_value) : value(new_value) {}

    /// Assignment: the assigned value is ignored
    this_t& operator=(value_type) { return *this; }

    //@{
    /// Conversion to the basic type: always returns the stored value
    operator value_type() const { return value; }
    operator const value_type&() const { return value; }
    //@}

  protected:
    value_type value; ///< the value stored for delivery
  };                  // class const_value_box

  /// @brief A constant iterator returning always the same value
  /// @tparam T type of the value returned by dereferenciation
  template <typename T>
  class value_const_iterator {
    typedef value_const_iterator<T> this_t; ///< alias for this type

  public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = T*;
    using reference = T&;

    /// Default constructor: use the default value
    value_const_iterator() : value() {}

    /// Constructor: value that will be returned
    value_const_iterator(value_type new_value) : value(new_value) {}

    /// Constructor: value to be returned and current iterator "position"
    value_const_iterator(value_type new_value, difference_type offset)
      : index(offset), value(new_value)
    {}

    /// Returns a copy of the stored value
    value_type operator*() const { return value; }

    /// Returns a copy of the stored value
    value_type operator[](difference_type) const { return value; }

    /// Increments the position of the iterator, returns the new position
    this_t& operator++()
    {
      ++index;
      return *this;
    }

    /// Increments the position of the iterator, returns the old position
    this_t operator++(int)
    {
      this_t copy(*this);
      ++index;
      return copy;
    }

    /// Decrements the position of the iterator, returns the new position
    this_t& operator--()
    {
      --index;
      return *this;
    }

    /// Decrements the position of the iterator, returns the old position
    this_t operator--(int)
    {
      this_t copy(*this);
      --index;
      return copy;
    }

    /// Increments the position of the iterator by the specified steps
    this_t& operator+=(difference_type ofs)
    {
      index += ofs;
      return *this;
    }

    /// Decrements the position of the iterator by the specified steps
    this_t& operator-=(difference_type ofs)
    {
      index -= ofs;
      return *this;
    }

    /// Returns an iterator pointing ahead of this one by the specified steps
    this_t operator+(difference_type ofs) const { return this_t(value, index + ofs); }

    /// Returns an iterator pointing behind this one by the specified steps
    this_t operator-(difference_type ofs) const { return this_t(value, index - ofs); }

    /// @brief Returns the distance between this iterator and the other
    /// @return how many steps this iterator is ahead of the other
    difference_type operator-(const this_t& iter) const { return index - iter.index; }

    //@{
    /// Comparison operators: determined by the position pointed by the iterators
    bool operator==(const this_t& as) const { return index == as.index; }
    bool operator!=(const this_t& as) const { return index != as.index; }

    bool operator<(const this_t& as) const { return index < as.index; }
    bool operator<=(const this_t& as) const { return index <= as.index; }
    bool operator>(const this_t& as) const { return index > as.index; }
    bool operator>=(const this_t& as) const { return index >= as.index; }
    //@}

  protected:
    difference_type index{0}; ///< (arbitrary) position pointed by the iterator
    value_type value;         ///< value to be returned when dereferencing
  };                          // class value_const_iterator<>

  /**
 * @brief Returns an iterator pointing ahead of this one by the specified steps
 * @param ofs number of steps ahead
 * @param iter base iterator
 */
  template <typename T>
  value_const_iterator<T> operator+(typename value_const_iterator<T>::difference_type ofs,
                                    value_const_iterator<T>& iter)
  {
    return {iter.value, iter.index + ofs};
  }

  /* // not ready yet...
template <typename T>
class value_iterator: public value_const_iterator<T> {
  using base_t = value_const_iterator<T>;
  using this_t = value_iterator<T>;
  using const_value_box<value_type> = value_box;
    public:

  using typename base_t::value_type;
  using typename base_t::reference;

  value_box operator* () const { return value_box(value); }
  value_box operator[] (difference_type) const { return value_box(value); }

}; // class value_iterator<>
*/

  //------------------------------------------------------------------------------
  //---  lar::range_t<SIZE>
  //---
  /**
 * @brief A range (interval) of integers
 * @tparam SIZE type of the indices (expected integral)
 *
 * Includes a first and an after-the-last value, and some relation metods.
 */
  template <typename SIZE>
  class range_t {
  public:
    typedef SIZE size_type;                 ///< type for the indices in the range
    typedef std::ptrdiff_t difference_type; ///< type for index difference

    static_assert(std::is_integral<size_type>::value, "range_t needs an integral type");

    size_type offset; ///< offset (absolute index) of the first element
    size_type last;   ///< offset (absolute index) after the last element

    /// Default constructor: empty range
    range_t() : offset(0), last(0) {}

    /// Constructor from first and last index
    range_t(size_type from, size_type to) : offset(from), last(std::max(from, to)) {}

    /// Sets the borders of the range
    void set(size_type from, size_type to)
    {
      offset = from;
      last = std::max(from, to);
    }

    /// Returns the first absolute index included in the range
    size_type begin_index() const { return offset; }

    /// Returns the first absolute index not included in the range
    size_type end_index() const { return last; }

    /// Returns the position within the range of the absolute index specified
    size_type relative_index(size_type index) const { return index - offset; }

    /// Returns the size of the range
    size_type size() const { return last - offset; }

    /// Moves the end of the range to fit the specified size
    void resize(size_type new_size) { last = offset + new_size; }

    /// Moves the begin of the range by the specified amount
    void move_head(difference_type shift) { offset += shift; }

    /// Moves the end of the range by the specified amount
    void move_tail(difference_type shift) { last += shift; }

    /// Returns whether the range is empty
    bool empty() const { return last <= offset; }

    /// Returns whether the specified absolute index is included in this range
    bool includes(size_type index) const { return (index >= offset) && (index < last); }

    /// Returns whether the specified range is completely included in this one
    bool includes(const range_t& r) const
    {
      return includes(r.begin_index()) && includes(r.end_index());
    }

    /// Returns if this and the specified range overlap
    bool overlap(const range_t& r) const
    {
      return (begin_index() < r.end_index()) && (end_index() > r.begin_index());
    }

    /// Returns if there are elements in between this and the specified range
    bool separate(const range_t& r) const
    {
      return (begin_index() > r.end_index()) || (end_index() < r.begin_index());
    }

    /// @brief Returns whether an index is within or immediately after this range
    ///
    /// Returns whether the specified absolute index is included in this range
    /// or is immediately after it (not before it!)
    bool borders(size_type index) const { return (index >= offset) && (index <= last); }

    //@{
    /// Sort: this range is smaller if its offset is smaller
    bool operator<(const range_t& than) const { return less(*this, than); }
    //@}

    /// Returns whether the specified range has our same offset and size
    bool operator==(const range_t& as) const { return (offset == as.offset) && (last == as.last); }

    /// Returns whether the range is valid (that is, non-negative size)
    bool is_valid() const { return last >= offset; }

    //@{
    /// Returns if a is "less" than b
    static bool less(const range_t& a, const range_t& b) { return a.offset < b.offset; }
    static bool less(const range_t& a, size_type b) { return a.offset < b; }
    static bool less(size_type a, const range_t& b) { return a < b.offset; }
    //@}

    /// Helper type to be used for binary searches
    typedef bool (*less_int_range)(size_type, const range_t& b);
  }; // class range_t

  // -----------------------------------------------------------------------------
  // ---  lar::sparse_vector<T>
  // ---

  // the price of having used subclasses extensively:
  template <typename T>
  class sparse_vector;

  namespace details {
    template <typename T>
    decltype(auto) make_const_datarange_t(typename sparse_vector<T>::datarange_t& r);
  } // namespace details

  /** ****************************************************************************
 * @brief A sparse vector
 * @tparam T type of data stored in the vector
 * @todo backward iteration; reverse iterators; iterator on non-void elements
 * only; iterator on non-void elements only, returning a pair (index;value)
 *
 * A sparse_vector is a container of items marked by consecutive indices
 * (that is, a vector like std::vector), where only non-zero elements are
 * actually stored.
 * The implementation is a container of ranges of non-zero consecutive values;
 * the zero elements are effectively not stored in the object, and a zero is
 * returned whenever they are accessed.
 * In the following, the regions of zeros between the non-zero ranges are
 * collectively called "the void".
 *
 * See sparse_vector_test.cc for examples of usage.
 *
 * Although some level of dynamic assignment is present, the class is not very
 * flexible and it is best assigned just once, by adding ranges (add_range(); or
 * by push_back(), which is less efficient).
 * While this class mimics a good deal of the STL vector interface, it is
 * *not* a std::vector and it does not support all the tricks of it.
 *
 * For the following, let's assume:
 * ~~~
 *   lar::sparse_vector<double> sv;
 *   std::vector<double> buffer;
 * ~~~
 *
 * Supported usage
 * ---------------
 *
 * ~~~
 *   for (const auto& value: sv) std::cout << " " << value;
 * ~~~
 * ~~~
 * lar::sparse_vector<double>::const_iterator iValue = sv.cbegin(), vend = sv.cend();
 * while (iSV != sv.end()) std::cout << " " << *(iSV++);
 * ~~~
 * ~~~
 * for (auto value: sv) { ... }
 * ~~~
 * Common iteration on all elements. Better to do a constant one.
 * The first two examples print the full content of the sparse vector, void
 * included.
 *
 * ---
 *
 * ~~~
 * sv[10] = 3.;
 * ~~~
 * Assign to an *existing* (not void) element. Assigning to void elements
 * is not supported, and there is no way to make an existing element to become
 * void (assigning 0 will yield to an existing element with value 0).
 *
 * ---
 *
 * ~~~
 * sv.set_at(10, 3.);
 * ~~~
 * Assign a value to an element. The element could be in the void; in any case,
 * after this call the element will not be in the void anymore (even if the
 * assigned value is zero; to cast a cell into the void, use unset_at()).
 *
 * ---
 *
 * ~~~
 * sv.add_range(20, buffer)
 * ~~~
 * ~~~
 * sv.add_range(20, buffer.begin(), buffer.end())
 * ~~~
 * ~~~
 * sv.add_range(20, std::move(buffer))
 * ~~~
 * Add the content of buffer starting at the specified position.
 * The last line will attempt to use the buffer directly (only happens if the
 * start of the new range -- 20 in the example -- is in the void and therefore a
 * new range will be added). The new range is merged with the existing ones when
 * needed, and it overwrites their content in case of overlap.
 * If the specified position is beyond the current end of the sparse vector,
 * the gap will be filled by void.
 *
 * ---
 *
 * ~~~
 * sv.append(buffer)
 * ~~~
 * ~~~
 * sv.append(buffer.begin(), buffer.end())
 * ~~~
 * ~~~
 * sv.append(std::move(buffer))
 * ~~~
 * Add the content of buffer at the end of the sparse vector.
 * The last line will attempt to use the buffer directly (only happens if the
 * end of the sparse vector is in the void and therefore a new range will be
 * added).
 *
 * ---
 *
 * ~~~
 * sv.resize(30)
 * ~~~
 * Resizes the sparse vector to the specified size. Truncation may occur, in
 * which case the data beyond the new size is removed. If an extension occurs
 * instead, the new area is void.
 *
 * ---
 *
 * ~~~
 * for (const lar::sparse_vector<double>::datarange_t& range: sv.get_ranges()) {
 *   size_t first_item = range.begin_index(); // index of the first item
 *   size_t nItems = range.size(); // number of items in this range
 *   for (double value: range) { ... }
 * }
 * ~~~
 * A sparse vector can be parsed range by range, skipping the void. Each range
 * object itself supports iteration.
 * Neither the content nor the shape of the ranges can be changed this way.
 *
 *
 * Possible future improvements
 * ----------------------------
 *
 * ~~~
 * sv[10] = 3.;
 * ~~~
 * is not supported if the vector is shorter than 11 (similarly to a
 * std::vector too), and not even if the item #10 is currently in the void.
 * This latter could be changed to create a new element/range; this would
 * require to ship the pointer to the container with the reference (the return
 * value of "sv[10]"), just in case an assignment will occur, or to create the
 * element regardless, like for std::map (but this would provoke a disaster
 * if the caller uses the non-constant operator[] for iteration).
 * So far, set_at() is the closest thing to it.
 *
 *
 * Non-supported usage
 * -------------------
 *
 * ~~~
 * sv.reserve(20);
 * ~~~
 * This has no clear meaning. A usage analogous to STL would precede a sequence
 * of push_back's. In that case, we should create a new empty range at the end
 * of the vector, and reserve that. Empty ranges are currently not allowed.
 * A replacement of this pattern is to create a new `std::vector`, reserve space
 * for it and fill it, and finally use `sparse_vector::append()`. If the end
 * of the vector is void, there will be no performance penalty, otherwise a
 * reserve + copy will happen.
 *
 * ---
 *
 * ~~~
 * for (auto& value: sv) { ... }
 * ~~~
 * In order to allow for assignment in an item which is currently not void,
 * the non-constant iterators do not dereference directly into a reference to
 * the vector element (which would be non-existent if in the void), but instead
 * into a lightweight object (still called "reference"). These objects are
 * semantically working as references, but they are formally rvalues (i.e., just
 * values, not C++ references), so they can't be assigned to references (like
 * "auto&"). Nevertheless they work as references and assigning to them does
 * change the original value. Currently assigning to void cells is not supported
 * (see above).
 *
 */
  template <typename T>
  class sparse_vector {
    typedef sparse_vector<T> this_t;
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    //  - - - public interface
  public:
    //  - - - types
    typedef T value_type; ///< type of the stored values
    typedef std::vector<value_type> vector_t;
    ///< type of STL vector holding this data
    typedef typename vector_t::size_type size_type; ///< size type
    typedef typename vector_t::difference_type difference_type;
    ///< index difference type
    typedef typename vector_t::pointer pointer;
    typedef typename vector_t::const_pointer const_pointer;
    //  typedef typename vector_t::reference reference;
    //  typedef typename vector_t::const_reference const_reference;

    // --- declarations only ---
    class reference;
    class const_reference;
    class iterator;
    class const_iterator;

    class datarange_t;
    class const_datarange_t;
    // --- ----------------- ---

    typedef std::vector<datarange_t> range_list_t;
    ///< type of sparse vector data
    typedef typename range_list_t::iterator range_iterator;
    ///< type of iterator over ranges
    typedef typename range_list_t::const_iterator range_const_iterator;
    ///< type of constant iterator over ranges

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    //  - - - public methods
    /// Default constructor: an empty vector
    sparse_vector() : nominal_size(0), ranges() {}

    /// Constructor: a vector with new_size elements in the void
    sparse_vector(size_type new_size) : nominal_size(0), ranges() { resize(new_size); }

    /**
   * @brief Constructor: a solid vector from an existing STL vector
   * @param from vector to copy data from
   * @param offset (default: 0) index the data starts from (preceeded by void)
   */
    sparse_vector(const vector_t& from, size_type offset = 0) : nominal_size(0), ranges()
    {
      add_range(offset, from.begin(), from.end());
    }

    /// Copy constructor: default
    sparse_vector(sparse_vector const&) = default;

    /// Move constructor
    sparse_vector(sparse_vector&& from)
      : nominal_size(from.nominal_size), ranges(std::move(from.ranges))
    {
      from.nominal_size = 0;
    }

    /// Copy assignment: default
    sparse_vector& operator=(sparse_vector const&) = default;

    /// Move assignment
    sparse_vector& operator=(sparse_vector&& from)
    {
      ranges = std::move(from.ranges);
      nominal_size = from.nominal_size;
      from.nominal_size = 0;
      return *this;
    } // operator= (sparse_vector&&)

    /**
   * @brief Constructor: a solid vector from an existing STL vector
   * @param from vector to move data from
   * @param offset (default: 0) index the data starts from (preceeded by void)
   */
    sparse_vector(vector_t&& from, size_type offset = 0) : nominal_size(0), ranges()
    {
      add_range(offset, std::move(from));
    }

    /// Destructor: default
    ~sparse_vector() = default;

    //  - - - STL-like interface
    /// Removes all the data, making the vector empty
    void clear()
    {
      ranges.clear();
      nominal_size = 0;
    }

    /// Returns the size of the vector
    size_type size() const { return nominal_size; }

    /// Returns whether the vector is empty
    bool empty() const { return size() == 0; }

    /// Returns the capacity of the vector (compatibility only)
    size_type capacity() const { return nominal_size; }

    //@{
    /// Resizes the vector to the specified size, adding void
    void resize(size_type new_size);
    /// Resizes the vector to the specified size, adding def_value
    void resize(size_type new_size, value_type def_value);
    //@}

    //@{
    /// Standard iterators interface
    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;
    const_iterator cbegin() const { return begin(); }
    const_iterator cend() const { return end(); }
    //@}

    /// Access to an element (read only)
    value_type operator[](size_type index) const;

    /// Access to an element (read/write for non-void elements only!)
    reference operator[](size_type index);

    //  - - - special interface

    ///@{ @name Cell test
    ///
    /// The methods in this group test the single vector cells.

    // --- element test
    /**
   * @brief Returns whether the specified position is void
   * @param index position of the cell to be tested
   * @throw out_of_range if index is not in the vector
   * @see is_back_void()
   */
    bool is_void(size_type index) const;

    /// @brief Returns whether the sparse vector ends with void
    /// @see is_void()
    bool back_is_void() const { return ranges.empty() || (ranges.back().end_index() < size()); }

    /// Returns the number of non-void cells
    size_type count() const;
    ///@}

    ///@{ @name Cell set
    ///
    /// The methods in this group access and/or change the cell values.
    /**
   * @brief Writes into an element (creating or expanding a range if needed)
   * @param index the index of the element to set
   * @param value the value to be set
   * @return a reference to the changed value
   * @see unset_at()
   *
   * Note that setting the value to zero will not cast the element into void.
   * Use unset_at for that.
   */
    value_type& set_at(size_type index, value_type value);

    /// Casts the element with the specified index into the void
    void unset_at(size_type index);

    //@{
    /// Adds one element to the end of the vector (zero values too)
    /// @param value value to be added
    void push_back(value_type value) { resize(size() + 1, value); }

    /**
   * @brief Adds one element to the end of the vector (if zero, just adds void)
   * @param value value to be added
   * @param thr threshold below which the value is considered zero
   *
   * If the threshold is strictly negative, all values are pushed back
   */
    void push_back(value_type value, value_type thr)
    {
      if (is_zero(value, thr))
        resize(size() + 1);
      else
        push_back(value);
    } // push_back()
    //@}

    //@{
    /**
   * @brief Copies data from a sequence between two iterators
   * @tparam ITER type of iterator
   * @param first iterator pointing to the first element to be copied
   * @param last iterator pointing after the last element to be copied
   *
   * The previous content of the sparse vector is lost.
   */
    template <typename ITER>
    void assign(ITER first, ITER last)
    {
      clear();
      append(first, last);
    }

    /**
   * @brief Copies data from a container
   * @tparam CONT type of container supporting the standard begin/end interface
   * @param new_data container with the data to be copied
   *
   * The previous content of the sparse vector is lost.
   */
    template <typename CONT>
    void assign(const CONT& new_data)
    {
      clear();
      append(new_data);
    }

    /**
   * @brief Moves data from a vector
   * @param new_data vector with the data to be moved
   *
   * The previous content of the sparse vector is lost.
   */
    void assign(vector_t&& new_data)
    {
      clear();
      append(std::move(new_data));
    }
    //@}

    ///@}

    // --- BEGIN Ranges ---------------------------------------------------------
    /**
   * @name Ranges
   *
   * A range is a contiguous region of the sparse vector which contains all
   * non-void values.
   *
   * A sparse vector is effectively a sorted collection of ranges.
   * This interface allows:
   * * iteration through all ranges (read only)
   * * random access to a range by index (read only)
   * * access to the data of a selected range (read/write)
   * * look-up of the range containing a specified index
   *     (read/write; use write with care!)
   * * addition (and more generically, combination with existing data) of a new
   *     range
   * * extension of the sparse vector by appending a range at the end of it
   * * remove the data of a range, making it void
   */
    /// @{

    // --- range location

    /// Returns the internal list of non-void ranges
    const range_list_t& get_ranges() const { return ranges; }
    auto iterate_ranges() -> decltype(auto);

    /// Returns the internal list of non-void ranges
    size_type n_ranges() const { return ranges.size(); }

    /// Returns the i-th non-void range (zero-based)
    const datarange_t& range(size_t i) const { return ranges[i]; }

    //@{
    /**
   * @brief Provides direct access to data of i-th non-void range (zero-based)
   * @param i index of the range
   * @return an object suitable for ranged-for iteration
   *
   * No information about the positioning of the range itself is provided,
   * which can be obtained with other means (e.g. `range(i).begin_index()`).
   * The returned object can be used in a ranged-for loop:
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
   * for (std::size_t iRange = 0; iRange < sv.n_ranges(); ++iRange) {
   *   for (auto& value: sv.range_data(iRange)) {
   *     v *= 2.0;
   *   }
   * }
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   * (with `sv` a `lar::sparse_vector` instance).
   *
   * While this is a somehow clumsier interface than `get_ranges()`, it allows,
   * using the non-`const` version, write access to the data elements.
   * It intentionally provides no write access to the location of each range,
   * though.
   */
    auto range_data(std::size_t i);
    auto range_data(std::size_t const i) const { return range_const_data(i); }
    //@}

    /// Like `range_data()` but with explicitly read-only access to data.
    auto range_const_data(std::size_t i) const;

    /// Returns a constant iterator to the first data range
    range_const_iterator begin_range() const { return ranges.begin(); }

    /// Returns a constant iterator to after the last data range
    range_const_iterator end_range() const { return ranges.end(); }

    //@{
    /**
   * @brief Returns an iterator to the range containing the specified index
   * @param index absolute index of the element to be sought
   * @return iterator to containing range, or get_ranges().end() if in void
   * @throw std::out_of_range if index is not in the vector
   * @see is_void()
   */
    range_const_iterator find_range_iterator(size_type index) const;
    range_iterator find_range_iterator(size_type index)
    {
      return ranges.begin() + find_range_number(index);
    }
    //@}

    /**
   * @brief Returns the number (0-based) of range containing `index`.
   * @param index absolute index of the element to be sought
   * @return index of containing range, or `n_ranges()` if in void
   * @throw std::out_of_range if index is not in the vector
   * @see is_void()
   */
    std::size_t find_range_number(size_type index) const
    {
      return find_range_iterator(index) - begin_range();
    }

    //@{
    /**
   * @brief Returns the range containing the specified index
   * @param index absolute index of the element to be sought
   * @return the containing range
   * @throw std::out_of_range if index is in no range (how appropriate!)
   * @see is_void()
   */
    const datarange_t& find_range(size_type index) const;
    datarange_t& find_range(size_type index);
    //@}

    /**
   * @brief Casts the whole range with the specified item into the void
   * @param index absolute index of the element whose range is cast to void
   * @return the range just voided
   * @throw std::out_of_range if index is not in the vector
   * @see unset(), make_void()
   */
    datarange_t make_void_around(size_type index);

    //@{
    /**
   * @brief Adds a sequence of elements as a range with specified offset
   * @tparam ITER type of iterator
   * @param offset where to add the elements
   * @param first iterator to the first element to be added
   * @param last iterator after the last element to be added
   * @return the range where the new data was added
   *
   * If the offset is beyond the current end of the sparse vector, void is
   * added before the new range.
   *
   * Existing ranges can be merged with the new data if they overlap.
   */
    template <typename ITER>
    const datarange_t& add_range(size_type offset, ITER first, ITER last);

    /**
   * @brief Copies the elements in container to a range with specified offset
   * @tparam CONT type of container supporting the standard begin/end interface
   * @param offset where to add the elements
   * @param new_data container holding the data to be copied
   * @return the range where the new data was added
   *
   * If the offset is beyond the current end of the sparse vector, void is
   * added before the new range.
   *
   * Existing ranges can be merged with the new data if they overlap.
   */
    template <typename CONT>
    const datarange_t& add_range(size_type offset, const CONT& new_data)
    {
      return add_range(offset, new_data.begin(), new_data.end());
    }

    /**
   * @brief Adds a sequence of elements as a range with specified offset
   * @param offset where to add the elements
   * @param new_data container holding the data to be moved
   * @return the range where the new data was added
   *
   * If the offset is beyond the current end of the sparse vector, void is
   * added before the new range.
   *
   * Existing ranges can be merged with the new data if they overlap.
   * If no merging happens, new_data vector is directly used as the new range
   * added; otherwise, it is just copied.
   */
    const datarange_t& add_range(size_type offset, vector_t&& new_data);
    //@}

    /// @{
    /**
   * @brief Combines a sequence of elements as a range with data at `offset`.
   * @tparam ITER type of iterator
   * @tparam OP combination operation
   * @param offset where to add the elements
   * @param first iterator to the first element to be added
   * @param last iterator after the last element to be added
   * @param op operation to be executed element by element
   * @param void_value (default: `value_zero`) the value to use for void cells
   * @return the range where the new data landed
   *
   * This is a more generic version of `add_range()`, where instead of
   * replacing the target data with the data in [ `first`, `last` [, the
   * existing data is combined with the one in that interval.
   * The operation `op` is a binary operation with signature equivalent to
   * `Data_t op(Data_t, Data_t)`, and the operation is equivalent to
   * `v[i + offset] = op(v[i + offset], *(first + offset))`: `op` is a binary
   * operation whose first operand is the existing value and the second one
   * is the one being provided.
   * If the cell `i + offset` is currently void, it will be created and the
   * value used in the combination will be `void_value`.
   *
   * If the offset is beyond the current end of the sparse vector, void is
   * added before the new range.
   *
   * Existing ranges can be merged with the new data if they overlap.
   */
    template <typename ITER, typename OP>
    const datarange_t& combine_range(size_type offset,
                                     ITER first,
                                     ITER last,
                                     OP&& op,
                                     value_type void_value = value_zero);

    /**
   * @brief Combines the elements in container with the data at `offset`.
   * @tparam CONT type of container supporting the standard begin/end interface
   * @tparam OP combination operation
   * @param offset where to add the elements
   * @param other container holding the data to be combined
   * @param op operation to be executed element by element
   * @param void_value (default: `value_zero`) the value to use for void cells
   * @return the range where the new data was added
   * @see `combine_range()`
   *
   * This is equivalent to `combine_range(size_type, ITER, ITER, OP, Data_t)`
   * using as the new data range the full content of `other` container.
   */
    template <typename CONT, typename OP>
    const datarange_t& combine_range(size_type offset,
                                     const CONT& other,
                                     OP&& op,
                                     value_type void_value = value_zero)
    {
      return combine_range(offset, other.begin(), other.end(), std::forward<OP>(op), void_value);
    }
    /// @}

    //@{
    /**
   * @brief Adds a sequence of elements as a range at the end of the vector.
   * @param first iterator to the first element to be added
   * @param last iterator after the last element to be added
   * @return the range where the new data was added
   *
   * The input range is copied at the end of the sparse vector.
   * If the end of the sparse vector was the end of a range, that range is
   * expanded, otherwise a new one is created.
   */
    template <typename ITER>
    const datarange_t& append(ITER first, ITER last)
    {
      return add_range(size(), first, last);
    }

    /**
   * @brief Adds a sequence of elements as a range at the end of the vector.
   * @param range_data contained holding the data to be copied or moved
   * @return the range where the new data was added
   *
   * The input data is copied at the end of the sparse vector.
   * If the end of the sparse vector was the end of a range, that range is
   * expanded, otherwise a new one is created.
   */
    template <typename CONT>
    const datarange_t& append(const CONT& range_data)
    {
      return add_range(size(), range_data);
    }

    /**
   * @brief Adds a sequence of elements as a range at the end of the vector.
   * @param range_data contained holding the data to be copied or moved
   * @return the range where the new data was added
   *
   * If there is a range at the end of the sparse vector, it will be expanded
   * with the new data.
   * Otherwise, this method will use the data vector directly as a new range
   * added at the end of the sparse vector.
   */
    const datarange_t& append(vector_t&& range_data)
    {
      return add_range(size(), std::move(range_data));
    }
    //@}

    //@{
    /**
   * @brief Turns the specified range into void
   * @param iRange iterator or index of range to be deleted
   * @return the range just voided
   * @see make_void(), unset_at()
   *
   * The range is effectively removed from the sparse vector, rendering void
   * the interval it previously covered.
   * The range object itself is returned (no copy is performed).
   *
   * The specified range must be valid. Trying to void an invalid range
   * (including `end_range()`) yields undefined behavior.
   */
    datarange_t void_range(range_iterator const iRange);
    datarange_t void_range(std::size_t const iRange) { return void_range(ranges.begin() + iRange); }
    //@}
    ///@}

    /**
   * @brief Returns if the vector is in a valid state
   *
   * The vector is in a valid state if:
   * - no ranges overlap or touch each other (a void gap must exist)
   * - no range is empty
   * - all ranges are sorted
   * - the size of the vector is not smaller than the sum of the size of
   *   the ranges plus the internal gaps
   * An invalid state can be the result of &ldquo;too smart&rdquo; use of this
   * class, or of a bug, which should be reported to the author.
   */
    bool is_valid() const;

    /// Makes all the elements from first and before last void
    void make_void(iterator first, iterator last);

    //@{
    /// Performs internal optimization, returns whether the object was changed
    bool optimize() { return optimize(min_gap()); }
    bool optimize(size_t) { return false; /* no optimization implemented yet */ }
    //@}

    ///@{ @name Static members for dealing with this type of value

    static constexpr value_type value_zero{0}; ///< a representation of 0

    /// Returns the module of the specified value
    static value_type abs(value_type v) { return (v < value_zero) ? -v : v; }

    /// Returns whether the value is exactly zero
    static value_type is_zero(value_type v) { return v == value_zero; }

    /// Returns whether the value is zero below a given threshold
    static value_type is_zero(value_type v, value_type thr) { return abs(v - value_zero) <= thr; }

    /// Returns whether two values are the same
    static value_type is_equal(value_type a, value_type b) { return is_zero(abs(a - b)); }

    /// Returns whether two values are the same below a given threshold
    static value_type is_equal(value_type a, value_type b, value_type thr)
    {
      return is_zero(abs(a - b), thr);
    }
    ///@}

    ///@{ @name Static members related to data size and optimization

    /// Returns the expected size taken by a vector of specified size
    static size_t expected_vector_size(size_t size);

    /// Minimum optimal gap between ranges (a guess)
    static size_t min_gap();

    /// Returns if merging the two specified ranges would save memory
    static bool should_merge(const typename datarange_t::base_t& a,
                             const typename datarange_t::base_t& b);
    ///@}

    // --------------------------------------------------------------------------
  protected:
    size_type nominal_size; ///< current size
    range_list_t ranges;    ///< list of ranges

    //@{
    /**
   * @brief Returns an iterator to the range after `index`.
   * @param index the absolute index
   * @return iterator to the next range not including index, or ranges.end()
   *         if none
   */
    range_iterator find_next_range_iter(size_type index)
    {
      return find_next_range_iter(index, ranges.begin());
    }
    range_const_iterator find_next_range_iter(size_type index) const
    {
      return find_next_range_iter(index, ranges.cbegin());
    }
    //@}
    //@{
    /**
   * @brief Returns an iterator to the range after `index`,
   *        or `ranges.end()` if none.
   * @param index the absolute index
   * @param rbegin consider only from this range on
   * @return iterator to the next range not including index, or `ranges.end()`
   *         if none
   */
    range_iterator find_next_range_iter(size_type index, range_iterator rbegin);
    range_const_iterator find_next_range_iter(size_type index, range_const_iterator rbegin) const;
    //@}

    //@{
    /**
   * @brief Returns an iterator to the range at or after `index`.
   * @param index the absolute index
   * @return iterator to the next range including index or after it,
   *         or ranges.end() if none
   * @see `find_next_range_iter()`
   *
   * If `index` is in a range, an iterator to that range is returned.
   * If `index` is in the void, an iterator to the next range after `index` is
   * returned. If there is no such range after `index`, `ranges.end()` is
   * returned.
   */
    range_iterator find_range_iter_at_or_after(size_type index);
    range_const_iterator find_range_iter_at_or_after(size_type index) const;
    //@}

    //@{
    /**
   * @brief Returns an iterator to the range no earlier than `index`,
            or `end()` if none.
   * @param index the absolute index
   * @return iterator to the range including index, or the next range if none
   *
   * The returned iterator points to a range that "borders" the specified index,
   * meaning that the cell at `index` is either within the range, or it is the
   * one immediately after that range.
   * If `index` is in the middle of the void, though (i.e. if the previous cell
   * is void), the next range is returned instead.
   * Finally, if there is no next range, `end_range()` is returned.
   *
   * The result may be also interpreted as follow: if the start of the returned
   * range is lower than `index`, then the cell at `index` belongs to that
   * range. Otherwise, it initiates its own range (but that range might end up
   * being contiguous to the next(.
   */
    range_iterator find_extending_range_iter(size_type index)
    {
      return find_extending_range_iter(index, ranges.begin());
    }
    range_const_iterator find_extending_range_iter(size_type index) const
    {
      return find_extending_range_iter(index, ranges.cbegin());
    }
    //@}

    //@{
    /**
   * @brief Returns an iterator to the range that contains the first non-void
   *        element after `index`, or `end()` if none.
   * @param index the absolute index
   * @param rbegin consider only from this range on
   * @return iterator to the next range not including index, or `ranges.end()`
   *         if none
   *
   * Note that the returned range can contain `index` as well.
   */
    range_iterator find_extending_range_iter(size_type index, range_iterator rbegin);
    range_const_iterator find_extending_range_iter(size_type index,
                                                   range_const_iterator rbegin) const;
    //@}

    /// Returns the size determined by the ranges already present
    size_type minimum_size() const { return ranges.empty() ? 0 : ranges.back().end_index(); }

    //@{
    /// Plug a new data range in the specified position; no check performed
    range_iterator insert_range(range_iterator iInsert, const datarange_t& data)
    {
      return data.empty() ? iInsert : ranges.insert(iInsert, data);
    }
    range_iterator insert_range(range_iterator iInsert, datarange_t&& data)
    {
      return data.empty() ? iInsert : ranges.insert(iInsert, std::move(data));
    }
    //@}

    /// Implementation detail of `add_range()`, with where to add the range.
    const datarange_t& add_range_before(size_type offset,
                                        vector_t&& new_data,
                                        range_iterator nextRange);

    /// Voids the starting elements up to index (excluded) of a given range
    range_iterator eat_range_head(range_iterator iRange, size_t index);

    /**
   * @brief Merges all the following contiguous ranges.
   * @param iRange iterator to the range to merge the following ones into
   * @return iterator to the merged range
   *
   * Starting from the range next to `iRange`, if that range is contiguous to
   * `iRange` the two are merged. The merging continues while there are
   * ranges contiguous to `iRange`. In the end, an iterator is returned pointing
   * to a range that has the same starting point that `iRange` had, and that is
   * not followed by a contiuguous range, since all contiguous ranges have been
   * merged into it.
   */
    datarange_t& merge_ranges(range_iterator iRange);

    /// Extends the vector size according to the last range
    size_type fix_size();

  }; // class sparse_vector<>

} // namespace lar

/**
 * @brief Prints a sparse vector into a stream
 * @tparam T template type of the sparse vector
 * @param out output stream
 * @param v the sparse vector to be written
 * @return the output stream (out)
 *
 * The output is in the form:
 * <pre>
 * Sparse vector of size ## with ## ranges:
 *   [min1 - max1] (size1) { elements of the first range }
 *   [min2 - max2] (size2) { elements of the second range }
 *   ...
 * </pre>
 */
template <typename T>
std::ostream& operator<<(std::ostream& out, const lar::sparse_vector<T>& v);

// -----------------------------------------------------------------------------
// --- sparse_vector::datarange_t definition
// ---

/// Range class, with range and data
template <typename T>
class lar::sparse_vector<T>::datarange_t : public range_t<size_type> {
public:
  typedef range_t<size_type> base_t; ///< base class

  typedef typename vector_t::iterator iterator;
  typedef typename vector_t::const_iterator const_iterator;

  /// Default constructor: an empty range
  datarange_t() : base_t(), values() {}

  /// Constructor: range initialized with 0
  datarange_t(const base_t& range) : base_t(range), values(range.size()) {}

  /// Constructor: offset and data
  template <typename ITER>
  datarange_t(size_type offset, ITER first, ITER last)
    : base_t(offset, offset + std::distance(first, last)), values(first, last)
  {}

  /// Constructor: offset and data as a vector (which will be used directly)
  datarange_t(size_type offset, vector_t&& data)
    : base_t(offset, offset + data.size()), values(data)
  {}

  //@{
  /// Returns an iterator to the specified absolute value (no check!)
  iterator get_iterator(size_type index) { return values.begin() + index - base_t::begin_index(); }
  const_iterator get_iterator(size_type index) const { return get_const_iterator(index); }
  const_iterator get_const_iterator(size_type index) const
  {
    return values.begin() + index - base_t::begin_index();
  }
  //@}

  //@{
  /// begin and end iterators
  iterator begin() { return values.begin(); }
  iterator end() { return values.end(); }
  const_iterator begin() const { return values.begin(); }
  const_iterator end() const { return values.end(); }
  const_iterator cbegin() const { return values.cbegin(); }
  const_iterator cend() const { return values.cend(); }
  //@}

  //@{
  /// Resizes the range (optionally filling the new elements with def_value)
  void resize(size_t new_size)
  {
    values.resize(new_size);
    fit_size_from_data();
  }
  void resize(size_t new_size, value_type def_value)
  {
    values.resize(new_size, def_value);
    fit_size_from_data();
  }
  //@}

  //@{
  /// Returns the value at the specified absolute index
  value_type& operator[](size_type index) { return values[base_t::relative_index(index)]; }
  const value_type& operator[](size_type index) const
  {
    return values[base_t::relative_index(index)];
  }
  //@}

  //@{
  /// Return the vector of data values
  const vector_t& data() const { return values; }
  //  vector_t& data() { return values; }
  //@}

  /**
   * @brief Appends the specified elements to this range.
   * @tparam ITER type of iterator of the range
   * @param index the starting point
   * @param first iterator to the first object to copy
   * @param last iterator after the last object to copy
   * @return the extended range
   */
  template <typename ITER>
  datarange_t& extend(size_type index, ITER first, ITER last);

  /**
   * @brief Moves the begin of this range
   * @param to_index absolute index to move the head to
   * @param def_value value to be inserted in case of expansion of the range
   */
  void move_head(size_type to_index, value_type def_value = value_zero);

  /**
   * @brief Moves the end of this range
   * @param to_index absolute index to move the tail to
   * @param def_value value to be inserted in case of expansion of the range
   */
  void move_tail(size_type to_index, value_type def_value = value_zero)
  {
    resize(base_t::relative_index(to_index), def_value);
  }

  /**
   * @brief Dumps the content of this data range into a stream.
   * @tparam Stream type of stream to sent the dump to
   * @param out stream to sent the dump to
   *
   * The output format is:
   *
   *     [min - max] (size) { values... }
   *
   * Output is on a single line, which is not terminated.
   */
  template <typename Stream>
  void dump(Stream&& out) const;

protected:
  vector_t values; ///< data in the range

  void fit_size_from_data() { base_t::resize(values.size()); }

}; // class datarange_t

/**
 * @brief A constant reference to a data range.
 *
 * Values in the range can be modified, but their position and number can not.
 */
template <typename T>
class lar::sparse_vector<T>::const_datarange_t : private datarange_t {
public:
  using iterator = typename datarange_t::iterator;
  using const_iterator = typename datarange_t::const_iterator;

  // `range_t` interface
  using datarange_t::begin_index;
  using datarange_t::borders;
  using datarange_t::empty;
  using datarange_t::end_index;
  using datarange_t::includes;
  using datarange_t::last;
  using datarange_t::offset;
  using datarange_t::overlap;
  using datarange_t::relative_index;
  using datarange_t::separate;
  using datarange_t::size;
  using datarange_t::operator<;
  using datarange_t::operator==;
  using datarange_t::is_valid;
  using datarange_t::less;
  using less_int_range = typename datarange_t::less_int_range;

  // `datarange_t` interface
  using datarange_t::get_iterator;
  decltype(auto) begin() { return datarange_t::begin(); }
  decltype(auto) end() { return datarange_t::end(); }
  //   using datarange_t::begin;
  //   using datarange_t::end;
  //   using datarange_t::cbegin;
  //   using datarange_t::cend;
  using datarange_t::operator[];
  using datarange_t::dump;

  /// Return the vector of data values (only constant access).
  const vector_t& data() const { return base().values; }

  ~const_datarange_t() = delete; // can't destroy; can cast into it though

protected:
  friend decltype(auto) details::make_const_datarange_t<T>(datarange_t&);

  datarange_t const& base() const { return static_cast<datarange_t const&>(*this); }
  datarange_t& base() { return static_cast<datarange_t&>(*this); }

  static void static_check() { static_assert(sizeof(const_datarange_t) == sizeof(datarange_t)); }

}; // lar::sparse_vector<T>::const_datarange_t

// -----------------------------------------------------------------------------
// --- sparse_vector iterators definition
// ---

/// Special little box to allow void elements to be treated as references.
template <typename T>
class lar::sparse_vector<T>::const_reference {
protected:
  const value_type* ptr;

public:
  const_reference(const value_type* pValue = 0) : ptr(pValue) {}
  const_reference(const value_type& value) : const_reference(&value) {}

  explicit operator value_type() const { return ptr ? *ptr : value_zero; }
  operator const value_type&() const { return ptr ? *ptr : value_zero; }
}; // lar::sparse_vector<T>::const_reference

/**
  * @brief A class representing a cell in a sparse vector.
  *
  * This class is a little box allowing assignment of values into it;
  * if the internal pointer is invalid (as in case of void cell),
  * dereferencing or assigning will provoke a segmentation fault.
  */
template <typename T>
class lar::sparse_vector<T>::reference : public const_reference {
  friend class iterator;

  // This object "disappears" when assigned to: either the assignment is not
  // possible, and then a segmentation fault will occur, or the return value
  // is an actual C++ reference to the assigned value.
  // The same is true when explicitly converting it to a reference.
public:
  reference(value_type* pValue = 0) : const_reference(pValue) {}
  reference(value_type& value) : reference(&value) {}

  reference& operator=(const reference&) = default;
  value_type& operator=(value_type v) { return const_cast<value_type&>(*const_reference::ptr) = v; }

  explicit operator value_type&() { return const_cast<value_type&>(*const_reference::ptr); }

protected:
  explicit reference(const const_reference& from) : const_reference(from) {}

}; // lar::sparse_vector<T>::reference

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/// Iterator to the sparse vector values
template <typename T>
class lar::sparse_vector<T>::const_iterator {
  //
  // This iterator fulfils the traits of an immutable forward iterator.
  //

protected:
  friend class container_t;

  typedef sparse_vector<T> container_t;
  typedef typename container_t::size_type size_type;
  typedef typename container_t::range_list_t::const_iterator ranges_const_iterator;

public:
  // so far, only forward interface is implemented;
  // backward is tricky...
  typedef std::forward_iterator_tag iterator_category;

  /// Namespace for special initialization
  struct special {
    class begin {};
    class end {};
  };

  // types
  typedef typename container_t::value_type value_type;
  typedef typename container_t::difference_type difference_type;
  typedef typename container_t::pointer pointer;
  typedef typename container_t::reference reference;
  // note that this is not out little box, it's the normal C++ constant
  // reference from the underlying vector
  typedef typename vector_t::const_reference const_reference;

  /// Default constructor, does not iterate anywhere
  const_iterator() : cont(nullptr), index(0), currentRange() {}

  /// Constructor from a container and a offset
  const_iterator(const container_t& c, size_type offset)
    : cont(&c), index(std::min(offset, c.size())), currentRange()
  {
    refresh_state();
  }

  /// Special constructor: initializes at the beginning of the container
  const_iterator(const container_t& c, const typename special::begin)
    : cont(&c), index(0), currentRange(c.get_ranges().begin())
  {}

  /// Special constructor: initializes at the end of the container
  const_iterator(const container_t& c, const typename special::end)
    : cont(&c), index(c.size()), currentRange(c.get_ranges().end())
  {}

  /// Random access
  const_reference operator[](size_type offset) const { return (*cont)[index + offset]; }

  /// Constant dereferenciation operator
  const_reference operator*() const;

  //@{
  /// Increment and decrement operators
  const_iterator& operator++();
  const_iterator operator++(int)
  {
    const_iterator copy(*this);
    const_iterator::operator++();
    return copy;
  }
  //@}

  //@{
  /// Increment and decrement operators
  const_iterator& operator+=(difference_type delta);
  const_iterator& operator-=(difference_type delta);
  const_iterator operator+(difference_type delta) const;
  const_iterator operator-(difference_type delta) const;
  //@}

  /// Distance operator
  difference_type operator-(const const_iterator& iter) const;

  //@{
  /// Iterator comparisons
  bool operator==(const const_iterator& as) const
  {
    return (cont == as.cont) && (index == as.index);
  }
  bool operator!=(const const_iterator& as) const
  {
    return (cont != as.cont) || (index != as.index);
  }
  bool operator<(const const_iterator& than) const
  {
    return (cont == than.cont) && (index < than.index);
  }
  bool operator>(const const_iterator& than) const
  {
    return (cont == than.cont) && (index > than.index);
  }
  bool operator<=(const const_iterator& than) const
  {
    return (cont == than.cont) && (index <= than.index);
  }
  bool operator>=(const const_iterator& than) const
  {
    return (cont == than.cont) && (index >= than.index);
  }
  //@}

  /// Returns the current range internal value; use it at your own risk!!
  range_const_iterator get_current_range() const { return currentRange; }

protected:
  //
  // Logic of the internals:
  // - cont provides the list of ranges
  // - index is the absolute index in the container
  // - currentRange is a pointer to the "current" range, cached for speed
  //
  // An iterator past-the-end has the index equal to the size of the
  // container it points to. Operations on it are undefined, except that
  // going backward by one decrement goes back to the container
  // (if not empty) TODO currently backward iteration is not supported
  //
  // When the iterator is pointing to an element within a range,
  // currentRange points to that range.
  // When the iterator is pointing into the void of the vector, currentRange
  // points to the range next to that void area, or end() if there is none.
  // When the iterator is past-the-end, currentRange is not defined.
  //
  // Conversely, when the index is equal (or greater) to the size of the
  // container, the iterator is not dereferenciable, the iterator points
  // past-the-end of the container.
  // When currentRange points to a valid range and the index
  // is before that range, the iterator is pointing to the void.
  // When currentRange points to a valid range and the index
  // is inside that range, the iterator is pointing to an item in that
  // range.
  // When currentRange points to a valid range, the index can't point beyond
  // that range.
  // When currentRange points to the past-to-last range, the iterator is
  // pointing to the void (past the last range).
  //
  const container_t* cont;            ///< pointer to the container
  size_type index;                    ///< pointer to the current value, as absolute index
  ranges_const_iterator currentRange; ///< pointer to the current (or next) range

  /// Reassigns the internal state according to the index
  void refresh_state();

}; // class lar::sparse_vector<T>::const_iterator

/**
 * @brief Iterator to the sparse vector values
 *
 * This iterator respects the traits of an immutable forward iterator,
 * EXCEPT that the iterator can be non-dereferenciable even when it's a
 * "past the end" iterator.
 * That is due to the fact that currently dereferencing (and assigning)
 * to a cell which is not in a range already is not supported yet
 * (it can be done with some complicate mechanism).
 */
template <typename T>
class lar::sparse_vector<T>::iterator : public const_iterator {
  typedef typename const_iterator::container_t container_t;
  friend typename const_iterator::container_t;

public:
  typedef typename const_iterator::reference reference;
  typedef typename const_iterator::const_reference const_reference;
  typedef typename const_iterator::special special;

  /// Default constructor, does not iterate anywhere
  iterator() : const_iterator() {}

  /// Constructor from a container and an offset
  iterator(container_t& c, size_type offset = 0) : const_iterator(c, offset) {}

  /// Special constructor: initializes at the beginning of the container
  iterator(const container_t& c, const typename special::begin _) : const_iterator(c, _) {}

  /// Special constructor: initializes at the end of the container
  iterator(const container_t& c, const typename special::end _) : const_iterator(c, _) {}

  /// Random access
  reference operator[](size_type offset) const
  {
    return (*const_iterator::cont)[const_iterator::index + offset];
  }

  //@{
  /// Increment and decrement operators
  iterator& operator++()
  {
    const_iterator::operator++();
    return *this;
  }
  iterator operator++(int _) { return const_iterator::operator++(_); }
  //@}

  //@{
  /// Increment and decrement operators
  iterator& operator+=(difference_type delta)
  {
    const_iterator::operator+=(delta);
    return *this;
  }
  iterator& operator-=(difference_type delta)
  {
    const_iterator::operator+=(delta);
    return *this;
  }
  iterator operator+(difference_type delta) const { return const_iterator::operator+(delta); }
  iterator operator-(difference_type delta) const { return const_iterator::operator-(delta); }
  //@}

  /// Dereferenciation operator (can't write non-empty elements!)
  reference operator*() const { return reference(const_iterator::operator*()); }

  //  /// Returns the current range internal value; use it at your own risk!!
  //  range_iterator get_current_range() const
  //    { return const_iterator::currentRange; }

protected:
  // also worth conversion
  iterator(const_iterator from) : const_iterator(from) {}

}; // class lar::sparse_vector<T>::iterator

// -----------------------------------------------------------------------------
// ---  implementation  --------------------------------------------------------
// -----------------------------------------------------------------------------
namespace lar::details {

  // --------------------------------------------------------------------------
  /// Enclosure to use two iterators representing a range in a range-for loop.
  template <typename BITER, typename EITER>
  class iteratorRange {
    BITER b;
    EITER e;

  public:
    iteratorRange(BITER const& b, EITER const& e) : b(b), e(e) {}
    auto const& begin() const { return b; }
    auto const& end() const { return e; }
    auto const& size() const { return std::distance(begin(), end()); }
  }; // iteratorRange()

  // --------------------------------------------------------------------------
  template <typename T>
  decltype(auto) make_const_datarange_t(typename sparse_vector<T>::datarange_t& r)
  {
    return static_cast<typename sparse_vector<T>::const_datarange_t&>(r);
  }

  // --------------------------------------------------------------------------
  template <typename T>
  class const_datarange_iterator {
    using const_datarange_t = typename sparse_vector<T>::const_datarange_t;
    using base_iterator = typename sparse_vector<T>::range_iterator;
    base_iterator it;

  public:
    // minimal set of features for ranged-for loops
    const_datarange_iterator() = default;
    const_datarange_iterator(base_iterator it) : it(it) {}

    const_datarange_iterator& operator++()
    {
      ++it;
      return *this;
    }
    const_datarange_t& operator*() const { return make_const_datarange_t<T>(*it); }
    bool operator!=(const_datarange_iterator const& other) const { return it != other.it; }
  };

  // --------------------------------------------------------------------------

} // namespace lar::details

//------------------------------------------------------------------------------
//--- sparse_vector implementation

template <typename T>
constexpr typename lar::sparse_vector<T>::value_type lar::sparse_vector<T>::value_zero;

template <typename T>
decltype(auto) lar::sparse_vector<T>::iterate_ranges()
{
  return details::iteratorRange(details::const_datarange_iterator<T>(ranges.begin()),
                                details::const_datarange_iterator<T>(ranges.end()));
} // lar::sparse_vector<T>::iterate_ranges()

template <typename T>
void lar::sparse_vector<T>::resize(size_type new_size)
{
  if (new_size >= size()) {
    nominal_size = new_size;
    return;
  }

  // truncating...
  range_iterator iLastRange = find_next_range_iter(new_size);
  ranges.erase(iLastRange, ranges.end());
  if (!ranges.empty()) {
    range_iterator iLastRange = ranges.end() - 1;
    if (new_size == iLastRange->begin_index())
      ranges.erase(iLastRange);
    else if (new_size < iLastRange->end_index())
      iLastRange->resize(new_size - iLastRange->begin_index());
  } // if we have ranges

  // formally resize
  nominal_size = new_size;
} // lar::sparse_vector<T>::resize()

template <typename T>
void lar::sparse_vector<T>::resize(size_type new_size, value_type def_value)
{
  if (new_size == size()) return;
  if (new_size > size()) {

    if (back_is_void()) // add a new range
      append(vector_t(new_size - size(), def_value));
    else // extend the last range
      ranges.back().resize(new_size - ranges.back().begin_index(), def_value);

    nominal_size = new_size;
    return;
  }
  // truncating is the same whether there is a default value or not
  resize(new_size);
} // lar::sparse_vector<T>::resize()

template <typename T>
inline typename lar::sparse_vector<T>::iterator lar::sparse_vector<T>::begin()
{
  return iterator(*this, typename iterator::special::begin());
}

template <typename T>
inline typename lar::sparse_vector<T>::iterator lar::sparse_vector<T>::end()
{
  return iterator(*this, typename iterator::special::end());
}

template <typename T>
inline typename lar::sparse_vector<T>::const_iterator lar::sparse_vector<T>::begin() const
{
  return const_iterator(*this, typename const_iterator::special::begin());
}

template <typename T>
inline typename lar::sparse_vector<T>::const_iterator lar::sparse_vector<T>::end() const
{
  return const_iterator(*this, typename const_iterator::special::end());
}

template <typename T>
typename lar::sparse_vector<T>::value_type lar::sparse_vector<T>::operator[](size_type index) const
{
  // first range not including the index
  range_const_iterator iNextRange = find_next_range_iter(index);

  // if not even the first range includes the index, we are in the void
  // (or in some negative index space, if index signedness allowed);
  if (iNextRange == ranges.begin()) return value_zero;

  // otherwise, let's take the previous range;
  // if it includes the index, we return its value;
  // or it precedes it, and our index is in the void: return zero
  const datarange_t& range(*--iNextRange);
  return (index < range.end_index()) ? range[index] : value_zero;
} // lar::sparse_vector<T>::operator[]

template <typename T>
typename lar::sparse_vector<T>::reference lar::sparse_vector<T>::operator[](size_type index)
{
  // first range not including the index
  range_iterator iNextRange = find_next_range_iter(index);

  // if not even the first range includes the index, we are in the void
  // (or in some negative index space, if index signedness allowed);
  if (iNextRange == ranges.begin()) return reference();

  // otherwise, let's take the previous range;
  // if it includes the index, we return its value;
  // or it precedes it, and our index is in the void: return zero
  datarange_t& range(*--iNextRange);
  return (index < range.end_index()) ? reference(range[index]) : reference();
} // lar::sparse_vector<T>::operator[]

template <typename T>
bool lar::sparse_vector<T>::is_void(size_type index) const
{
  if (ranges.empty() || (index >= size())) throw std::out_of_range("empty sparse vector");
  // range after the index:
  range_const_iterator iNextRange = find_next_range_iter(index);
  return ((iNextRange == ranges.begin()) || ((--iNextRange)->end_index() <= index));
} // lar::sparse_vector<T>::is_void()

template <typename T>
inline typename lar::sparse_vector<T>::size_type lar::sparse_vector<T>::count() const
{
  return std::accumulate(begin_range(),
                         end_range(),
                         size_type(0),
                         [](size_type s, const datarange_t& rng) { return s + rng.size(); });
} // count()

template <typename T>
typename lar::sparse_vector<T>::value_type& lar::sparse_vector<T>::set_at(size_type const index,
                                                                          value_type value)
{
  // first range not including the index
  range_iterator iNextRange = find_next_range_iter(index);

  // if not even the first range includes the index, we are in the void
  // (or in some negative index space, if index signedness allowed);
  if (iNextRange != ranges.begin()) {
    // otherwise, let's take the previous range;
    // if it includes the index, we set the existing value;
    // or it precedes it, and our index is in the void, add the value as a
    // range
    datarange_t& range(*std::prev(iNextRange));
    if (index < range.end_index()) return range[index] = value;
  }
  // so we are in the void; add the value as a new range
  return const_cast<datarange_t&>(add_range(index, {value}))[index];
} // lar::sparse_vector<T>::set_at()

template <typename T>
void lar::sparse_vector<T>::unset_at(size_type index)
{
  // first range not including the index
  range_iterator iNextRange = find_next_range_iter(index);

  // if not even the first range includes the index, we are in the void
  // (or in some negative index space, if index signedness allowed);
  if (iNextRange == ranges.begin()) return;

  // otherwise, let's take the previous range;
  // or it precedes the index, and our index is in the void
  datarange_t& range(*--iNextRange);
  if (index >= range.end_index()) return; // void already

  // it includes the index:
  // - if it's a one-element range, remove the range
  if (range.size() == 1) ranges.erase(iNextRange);
  // - if it's on a border, resize the range
  else if (index == range.begin_index())
    range.move_head(index + 1);
  else if (index == range.end_index() - 1)
    range.move_tail(index);
  // - if it's inside, break the range in two
  else if (index < range.end_index()) {
    // we are going to put a hole in a range;
    // this effectively creates two ranges;
    // first create the rightmost range, and add it to the list
    ranges.emplace(
      ++iNextRange, index + 1, range.begin() + range.relative_index(index + 1), range.end());
    // then cut the existing one
    range.move_tail(index);
  }
} // lar::sparse_vector<T>::unset_at()

template <typename T>
auto lar::sparse_vector<T>::range_data(std::size_t const i)
{
  auto& r = ranges[i];
  return details::iteratorRange(r.begin(), r.end());
}

template <typename T>
auto lar::sparse_vector<T>::range_const_data(std::size_t const i) const
{
  auto& r = ranges[i];
  return details::iteratorRange(r.cbegin(), r.cend());
}

template <typename T>
typename lar::sparse_vector<T>::range_const_iterator lar::sparse_vector<T>::find_range_iterator(
  size_type index) const
{
  if (ranges.empty()) throw std::out_of_range("empty sparse vector");
  // range after the index:
  range_const_iterator iNextRange = find_next_range_iter(index);
  return ((iNextRange == ranges.begin()) || (index >= (--iNextRange)->end_index())) ? ranges.end() :
                                                                                      iNextRange;
} // lar::sparse_vector<T>::find_range_iterator() const

template <typename T>
const typename lar::sparse_vector<T>::datarange_t& lar::sparse_vector<T>::find_range(
  size_type index) const
{
  if (ranges.empty()) throw std::out_of_range("empty sparse vector");
  // range on the index:
  range_const_iterator iNextRange = find_range_iterator(index);
  if (iNextRange == ranges.end()) throw std::out_of_range("index in no range of the sparse vector");
  return *iNextRange;
} // lar::sparse_vector<T>::find_range() const

template <typename T>
inline typename lar::sparse_vector<T>::datarange_t& lar::sparse_vector<T>::find_range(
  size_type index)
{
  return const_cast<datarange_t&>(const_cast<const this_t*>(this)->find_range(index));
} // lar::sparse_vector<T>::find_range()

template <typename T>
auto lar::sparse_vector<T>::make_void_around(size_type index) -> datarange_t
{
  if (ranges.empty() || (index >= size())) throw std::out_of_range("empty sparse vector");
  // range after the index:
  range_iterator iNextRange = find_next_range_iter(index);
  if ((iNextRange == ranges.begin()) || ((--iNextRange)->end_index() <= index)) { return {}; }
  return void_range(iNextRange);
} // lar::sparse_vector<T>::make_void_around()

template <typename T>
template <typename ITER>
const typename lar::sparse_vector<T>::datarange_t&
lar::sparse_vector<T>::add_range(size_type offset, ITER first, ITER last)
{
  // insert the new range before the existing range which starts after offset
  range_iterator iInsert = find_next_range_iter(offset);

  // is there a range before this, which includes the offset?
  if ((iInsert != ranges.begin()) && std::prev(iInsert)->borders(offset)) {
    // then we should extend it
    (--iInsert)->extend(offset, first, last);
  }
  else {
    // no range before the insertion one includes the offset of the new range;
    // ... we need to add it as a new range
    iInsert = insert_range(iInsert, {offset, first, last});
  }
  return merge_ranges(iInsert);
} // lar::sparse_vector<T>::add_range<ITER>()

template <typename T>
const typename lar::sparse_vector<T>::datarange_t& lar::sparse_vector<T>::add_range(
  size_type offset,
  vector_t&& new_data)
{
  // insert the new range before the existing range which starts after offset
  return add_range_before(
    offset,
    std::move(new_data),
    std::upper_bound(ranges.begin(),
                     ranges.end(),
                     offset,
                     typename datarange_t::less_int_range(datarange_t::less)));

} // lar::sparse_vector<T>::add_range(vector)

template <typename T>
template <typename ITER, typename OP>
auto lar::sparse_vector<T>::combine_range(size_type offset,
                                          ITER first,
                                          ITER last,
                                          OP&& op,
                                          value_type void_value /* = value_zero */
                                          ) -> const datarange_t&
{
  /*
   * This is a complicate enough task, that we go brute force:
   * 1) combine all the input elements within the datarange where offset falls
   *    in
   * 2) create a new datarange combining void with the remaining input elements
   * 3) if the void area is over before the input elements are, repeat steps
   *    (1) and (2) with the updated offset
   * 4) at this point we'll have a train of contiguous ranges, result of
   *    combination of the input elements alternating with existing elements
   *    and with void cells: apply the regular merge algorithm
   *
   */

  auto src = first;
  auto const insertionPoint = offset; // saved for later
  auto destRange = find_range_iter_at_or_after(offset);
  while (src != last) {

    //
    // (1) combine all the input elements within the datarange including offset
    //     (NOT extending the range)
    //
    if ((destRange != end_range()) && destRange->includes(offset)) {
      // combine input data until this range is over (or input data is over)
      auto dest = destRange->get_iterator(offset);

      auto const end = destRange->end();
      while (src != last) {
        *dest = op(*dest, *src);
        ++src;
        ++offset;
        if (++dest == end) break;
      } // while
      if (src == last) break;
      offset = destRange->end_index();
      ++destRange;
    } // if

    //
    // (2) create a new datarange combining void with input elements
    //
    // at this point, offset is in the void, we do have more input data,
    // and `destRange` does _not_ contain the current insertion offset;
    // we fill as much void as we can with data, creating a new range.
    // When to stop? at the beginning of the next range, or when data is over
    //
    size_type const newRangeSize =
      (destRange == end_range()) ? std::distance(src, last) : (destRange->begin_index() - offset);

    // prepare the data (we'll plug it in directly)
    vector_t combinedData;
    combinedData.reserve(newRangeSize);
    size_type i = 0;
    while (i++ < newRangeSize) {
      combinedData.push_back(op(void_value, *src));
      if (++src == last) break; // no more data
    }
    // move the data as a new range inserted before the next range we just found
    // return value is the iterator to the inserted range
    destRange = insert_range(destRange, {offset, std::move(combinedData)});

    //
    // (3) if there is more input, repeat steps (1) and (2) with updated offset
    //
    offset = destRange->end_index();
    ++destRange;
  } // while

  //
  // (4) apply the regular merge algorithm;
  //     since we did not extend existing ranges, it may happen that we have
  //     created a new range at `insertionPoint` contiguous to the previous one;
  //     so we take care of checking that too
  //
  return merge_ranges(find_extending_range_iter(insertionPoint == 0 ? 0 : insertionPoint - 1));

} // lar::sparse_vector<T>::combine_range<ITER>()

template <typename T>
void lar::sparse_vector<T>::make_void(iterator first, iterator last)
{
  // iterators have in "currentRange" either the range they point into,
  // or the range next to the void they point to

  if ((first.cont != this) || (last.cont != this)) {
    throw std::runtime_error("lar::sparse_vector::make_void(): iterators from alien container");
  }
  // if first is after next, no range is identified
  if (first >= last) return;

  range_iterator first_range = ranges.begin() + (first.get_current_range() - ranges.begin()),
                 last_range = ranges.begin() + (last.get_current_range() - ranges.begin());

  //--- "first": void up to this iterator ---
  // if first in the last void region, there is nothing to erase
  if (first_range == ranges.end()) return;

  // if first is in the middle of a valid range instead, we have to resize it
  if (first.index > first_range->begin_index()) {
    if (first_range == last_range) {
      // we are going to erase a subset of a range;
      // this effectively creates two ranges;
      // first create the rightmost (last) range, and add it to the list
      last_range = ranges.emplace(++last_range,
                                  last.index,
                                  first_range->begin() + first_range->relative_index(last.index),
                                  first_range->end());
      // then cut the existing one
      first_range->move_tail(first.index);
      return;
    }
    first_range->move_tail(first.index);
    ++first_range; // from next range on, start voiding
  }

  //--- "last"
  // if the index is inside a range, remove up to it
  if ((last_range != ranges.end()) && (last.index > last_range->begin_index()))
    eat_range_head(last_range, last.index);

  // finally, remove entirely the ranges in between
  ranges.erase(first_range, last_range);
} // lar::sparse_vector<T>::make_void()

template <typename T>
auto lar::sparse_vector<T>::void_range(range_iterator iRange) -> datarange_t
{
  auto r{std::move(*iRange)}; // triggering move constructor
  ranges.erase(iRange);       // the emptied range is removed from vector
  return r;                   // returning it as a temporary avoids copies
} // lar::sparse_vector<T>::void_range()

template <typename T>
bool lar::sparse_vector<T>::is_valid() const
{
  // a sparse vector with no non-null elements can't be detected invalid
  if (ranges.empty()) return true;

  range_const_iterator iNext = ranges.begin(), rend = ranges.end();
  while (iNext != rend) {
    range_const_iterator iRange = iNext++;
    if (iRange->empty()) return false;
    if (iNext != rend) {
      if (!(*iRange < *iNext)) return false;
      if (!iRange->separate(*iNext)) return false;
    }
  } // while
  if (nominal_size < ranges.back().end_index()) return false;
  return true;
} // lar::sparse_vector<T>::is_valid()

// --- private methods

template <typename T>
typename lar::sparse_vector<T>::range_iterator lar::sparse_vector<T>::find_next_range_iter(
  size_type index,
  range_iterator rbegin)
{
  // this range has the offset (first index) above the index argument:
  return std::upper_bound(
    rbegin, ranges.end(), index, typename datarange_t::less_int_range(datarange_t::less));
} // lar::sparse_vector<T>::find_next_range_iter()

template <typename T>
typename lar::sparse_vector<T>::range_const_iterator lar::sparse_vector<T>::find_next_range_iter(
  size_type index,
  range_const_iterator rbegin) const
{
  // this range has the offset (first index) above the index argument:
  return std::upper_bound(
    rbegin, ranges.end(), index, typename datarange_t::less_int_range(datarange_t::less));
} // lar::sparse_vector<T>::find_next_range_iter() const

template <typename T>
typename lar::sparse_vector<T>::range_const_iterator
lar::sparse_vector<T>::find_range_iter_at_or_after(size_type index) const
{
  // this range has the offset (first index) above the index argument:
  auto after = find_next_range_iter(index);
  // if the previous range exists and contains index, that's the one we want
  return ((after != ranges.begin()) && (index < std::prev(after)->end_index())) ? std::prev(after) :
                                                                                  after;
} // lar::sparse_vector<T>::find_range_iter_at_or_after() const

template <typename T>
typename lar::sparse_vector<T>::range_iterator lar::sparse_vector<T>::find_range_iter_at_or_after(
  size_type index)
{
  // this range has the offset (first index) above the index argument:
  auto after = find_next_range_iter(index);
  // if the previous range exists and contains index, that's the one we want
  return ((after != ranges.begin()) && (index < std::prev(after)->end_index())) ? std::prev(after) :
                                                                                  after;
} // lar::sparse_vector<T>::find_range_iter_at_or_after()

template <typename T>
typename lar::sparse_vector<T>::range_iterator lar::sparse_vector<T>::find_extending_range_iter(
  size_type index,
  range_iterator rbegin)
{
  // this range has the offset (first index) above the index argument:
  auto it = find_next_range_iter(index, rbegin);
  // if index were not void, would it belong to the previous range?
  // if so, the previous range is the one we want
  return ((it != rbegin) && std::prev(it)->borders(index)) ? std::prev(it) : it;
} // lar::sparse_vector<T>::find_extending_range_iter()

template <typename T>
typename lar::sparse_vector<T>::range_const_iterator
lar::sparse_vector<T>::find_extending_range_iter(size_type index, range_const_iterator rbegin) const
{
  // this range has the offset (first index) above the index argument:
  auto it = find_next_range_iter(index, rbegin);
  // if index were not void, would it belong to the previous range?
  // if so, the previus range is the one we want
  return ((it != rbegin) && std::prev(it)->borders(index)) ? std::prev(it) : it;
} // lar::sparse_vector<T>::find_extending_range_iter() const

template <typename T>
const typename lar::sparse_vector<T>::datarange_t& lar::sparse_vector<T>::add_range_before(
  size_type offset,
  vector_t&& new_data,
  range_iterator nextRange)
{
  // insert the new range before the existing range which starts after offset
  range_iterator iInsert = nextRange;

  // is there a range before this, which includes the offset?
  if ((iInsert != ranges.begin()) && (iInsert - 1)->borders(offset)) {
    // then we should extend it
    (--iInsert)->extend(offset, new_data.begin(), new_data.end());
  }
  else {
    // no range before the insertion one includes the offset of the new range;
    // ... we need to add it as a new range;
    // it is not really clear to me [GP] why I need a std::move here, since
    // new_data is a rvalue already; in doubt, I have painted all this kind
    // of constructs with move()s, just in case
    iInsert = insert_range(iInsert, {offset, std::move(new_data)});
  }
  return merge_ranges(iInsert);
} // lar::sparse_vector<T>::add_range_before(vector, iterator)

template <typename T>
typename lar::sparse_vector<T>::datarange_t& lar::sparse_vector<T>::merge_ranges(
  range_iterator iRange)
{
  range_iterator iNext = iRange + 1;
  while (iNext != ranges.end()) {
    if (!iRange->borders(iNext->begin_index())) break;
    // iRange content dominates, but if iNext has data beyond it,
    // then we copy it
    if (iNext->end_index() > iRange->end_index()) {
      iRange->extend(iRange->end_index(), iNext->get_iterator(iRange->end_index()), iNext->end());
    }
    iNext = ranges.erase(iNext);
  } // while
  fix_size();
  return *iRange;
} // lar::sparse_vector<T>::merge_ranges()

template <typename T>
typename lar::sparse_vector<T>::range_iterator lar::sparse_vector<T>::eat_range_head(
  range_iterator iRange,
  size_t index)
{
  if (index <= iRange->begin_index()) return iRange;
  if (index >= iRange->end_index()) return ranges.erase(iRange);
  iRange->move_head(index);
  return iRange;
} // lar::sparse_vector<T>::eat_range_head()

template <typename T>
typename lar::sparse_vector<T>::size_type lar::sparse_vector<T>::fix_size()
{
  if (!ranges.empty()) nominal_size = std::max(nominal_size, ranges.back().end_index());
  return nominal_size;
} // lar::sparse_vector<T>::fix_size()

// --- static methods

template <typename T>
inline size_t lar::sparse_vector<T>::expected_vector_size(size_t size)
{
  // apparently, a chunk of heap memory takes at least 32 bytes;
  // that means that a vector of 1 or 5 32-bit integers takes the same
  // space; the overhead appears to be 8 bytes, which can be allocated
  return sizeof(vector_t) + std::max(size_t(32), (alignof(datarange_t) * size + 8));
} // lar::sparse_vector<T>::expected_vector_size()

template <typename T>
inline size_t lar::sparse_vector<T>::min_gap()
{
  // we assume here that there is no additional overhead by alignment;
  // the gap adds the space of another datarange_t, including the vector,
  // its data and overhead from heap (apparently, 8 bytes);
  //
  return (sizeof(datarange_t) + 8) / sizeof(value_type) + 1; // round up
} // lar::sparse_vector<T>::min_gap()

template <typename T>
inline bool lar::sparse_vector<T>::should_merge(const typename datarange_t::base_t& a,
                                                const typename datarange_t::base_t& b)
{
  size_type gap_size = (a < b) ? b.begin_index() - a.begin_index() - a.size() :
                                 a.begin_index() - b.begin_index() - b.size();
  return expected_vector_size(a.size() + b.size() + gap_size) <=
         expected_vector_size(a.size()) + expected_vector_size(b.size());
} // lar::sparse_vector<T>::should_merge()

// --- non-member functions
template <typename T>
std::ostream& operator<<(std::ostream& out, const lar::sparse_vector<T>& v)
{

  out << "Sparse vector of size " << v.size() << " with " << v.get_ranges().size() << " ranges:";
  typename lar::sparse_vector<T>::range_const_iterator iRange = v.begin_range(),
                                                       rend = v.end_range();
  while (iRange != rend) {
    out << "\n  ";
    iRange->dump(out);
    /*
    out << "\n  [" << iRange->begin_index() << " - " << iRange->end_index()
      << "] (" << iRange->size() << "):";
    typename lar::sparse_vector<T>::datarange_t::const_iterator
      iValue = iRange->begin(), vend = iRange->end();
    while (iValue != vend) out << " " << (*(iValue++));
    */
    ++iRange;
  } // for
  return out << std::endl;
} // operator<< (ostream, sparse_vector<T>)

// -----------------------------------------------------------------------------
// --- lar::sparse_vector<T>::datarange_t implementation
// ---
template <typename T>
template <typename ITER>
typename lar::sparse_vector<T>::datarange_t&
lar::sparse_vector<T>::datarange_t::extend(size_type index, ITER first, ITER last)
{
  size_type new_size =
    std::max(base_t::relative_index(index) + std::distance(first, last), base_t::size());
  base_t::resize(new_size);
  values.resize(new_size);
  std::copy(first, last, get_iterator(index));
  return *this;
} // lar::sparse_vector<T>::datarange_t::extend()

template <typename T>
void lar::sparse_vector<T>::datarange_t::move_head(size_type to_index,
                                                   value_type def_value /* = value_zero */)
{
  difference_type delta = to_index - base_t::begin_index();
  if (delta == 0) return;
  base_t::move_head(delta);
  if (delta > 0) // shrink
    values.erase(values.begin(), values.begin() + delta);
  else { // extend
    values.insert(values.begin(),
                  value_const_iterator<value_type>(def_value),
                  value_const_iterator<value_type>(def_value) - delta);
  }
} // lar::sparse_vector<T>::datarange_t::move_head()

template <typename T>
template <typename Stream>
void lar::sparse_vector<T>::datarange_t::dump(Stream&& out) const
{
  out << "[" << this->begin_index() << " - " << this->end_index() << "] (" << this->size()
      << "): {";
  for (auto const& v : this->values)
    out << " " << v;
  out << " }";
} // lar::sparse_vector<T>::datarange_t::dump()

// -----------------------------------------------------------------------------
// --- lar::sparse_vector<T>::const_iterator implementation
// ---
template <typename T>
typename lar::sparse_vector<T>::const_iterator& lar::sparse_vector<T>::const_iterator::operator++()
{
  // no container, not doing anything;
  // index beyond the end: stays there
  if (!cont || (index >= cont->size())) return *this;

  // increment the internal index
  ++index;

  // were we in a valid range?
  if (currentRange != cont->ranges.end()) {
    // if the new index is beyond the current range, pick the next
    if (currentRange->end_index() <= index) ++currentRange;
  }
  // if we have no valid range, we are forever in the void
  return *this;
} // lar::sparse_vector<T>::iterator::operator++()

template <typename T>
typename lar::sparse_vector<T>::const_iterator::const_reference
  lar::sparse_vector<T>::const_iterator::operator*() const
{
  // no container, no idea what to do
  if (!cont) throw std::out_of_range("iterator to no sparse vector");

  // index beyond the end: can't return any reference
  if (index >= cont->size()) return value_zero;

  // are we in a valid range? if not, we are past the last range
  if (currentRange == cont->ranges.end()) return value_zero;

  // if the index is beyond the current range, we are in the void
  if (index < currentRange->begin_index()) return value_zero;

  return (*currentRange)[index];
} // lar::sparse_vector<T>::const_iterator::operator*()

template <typename T>
typename lar::sparse_vector<T>::const_iterator& lar::sparse_vector<T>::const_iterator::operator+=(
  difference_type delta)
{
  if (delta == 1) return this->operator++();
  index += delta;
  if ((currentRange == cont->ranges.end()) || !currentRange->includes(index)) refresh_state();
  return *this;
} // lar::sparse_vector<T>::const_iterator::operator+=()

template <typename T>
inline typename lar::sparse_vector<T>::const_iterator& lar::sparse_vector<T>::const_iterator::
operator-=(difference_type delta)
{
  return this->operator+=(-delta);
}

template <typename T>
typename lar::sparse_vector<T>::const_iterator lar::sparse_vector<T>::const_iterator::operator+(
  difference_type delta) const
{
  if ((currentRange == cont->ranges.end()) || !currentRange->includes(index + delta))
    return const_iterator(*cont, index + delta);
  const_iterator iter(*this);
  iter.index += delta;
  return iter;
} // lar::sparse_vector<T>::const_iterator::operator+()

template <typename T>
inline typename lar::sparse_vector<T>::const_iterator lar::sparse_vector<T>::const_iterator::
operator-(difference_type delta) const
{
  return this->operator+(-delta);
}

/// distance operator
template <typename T>
inline typename lar::sparse_vector<T>::const_iterator::difference_type
lar::sparse_vector<T>::const_iterator::operator-(const const_iterator& iter) const
{
  if (cont != iter.cont) {
    throw std::runtime_error("lar::sparse_vector::const_iterator:"
                             " difference with alien iterator");
  }
  return index - iter.index;
} // lar::sparse_vector<T>::const_iterator::operator-(const_iterator)

template <typename T>
void lar::sparse_vector<T>::const_iterator::refresh_state()
{
  // update the currentRange
  // currentRange is the range including the current item, or next to it
  if (cont) {
    // the following is actually the range after the index:
    currentRange = cont->find_next_range_iter(index);
    // if the index is inside the previous index, then we want to move there:
    if (currentRange != cont->ranges.begin()) {
      if ((currentRange - 1)->end_index() > index) --currentRange;
    }
  }
  else {
    currentRange = {};
  }
} // lar::sparse_vector<T>::const_iterator::refresh_state()

// -----------------------------------------------------------------------------
// --- lar::sparse_vector<T>::iterator implementation
// ---
//
// nothing new so far
//

#endif // LARDATAOBJ_UTILITIES_SPARSE_VECTOR_H
