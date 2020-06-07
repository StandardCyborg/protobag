#pragma once

#include <vector>

namespace protobag {

// itertools.product(), but for C++
// https://docs.python.org/2/library/itertools.html#itertools.product
class IterProducts {
public:

  struct MaybeProduct {
    std::vector<size_t> indices;
    
    bool IsEndOfSequence() const { return indices.empty(); }
    
    static MaybeProduct EndOfSequence() { return {}; }
    
    static MaybeProduct First(size_t num_pools) {
      return {.indices = std::vector<size_t>(num_pools, 0)};
    }
  };

  explicit IterProducts(std::vector<size_t> &&pool_sizes) {
    _pool_sizes = std::move(pool_sizes);
  }

  const MaybeProduct &GetNext();

protected:
  std::vector<size_t> _pool_sizes;
  MaybeProduct _next;

  bool NoMoreProducts() const { return _pool_sizes.empty(); }
  void SetNoMoreProducts() { 
    _pool_sizes.clear();
    _next = MaybeProduct::EndOfSequence();
  }
  
  size_t NumPools() const { return _pool_sizes.size(); }

  bool HaveEmptyPool() const {
    bool have_empty_pool = false;
    for (const auto &pool_size : _pool_sizes) {
      have_empty_pool |= (pool_size == 0);
    }
    return have_empty_pool;
  }

};



inline const IterProducts::MaybeProduct &IterProducts::GetNext() {
  // Return EndOfSequence forever or init
  if (NoMoreProducts()) {
    static const auto eos = MaybeProduct::EndOfSequence();
    return eos;
  } else if (_next.IsEndOfSequence()) {
    // Can we init?
    if (HaveEmptyPool()) {
      SetNoMoreProducts();
      static const auto eos = MaybeProduct::EndOfSequence();
      return eos;
    } else {
      _next = MaybeProduct::First(NumPools());
      return _next;
    }
  }

  // Compute next
  bool carry = true;
    // To start, we need to carry an increment into the first pool
  for (size_t p = 0; p < NumPools(); ++p) {
    if (carry) {
      { _next.indices[p] += 1; carry = false; } // do the carry
      if (_next.indices[p] == _pool_sizes[p]) {
        // Reset this pool, carry the increment into next pool
        _next.indices[p] = 0;
        carry = true;
      } else {
        break;
      }
    } else {
      break;
    }
  }

  if (carry) {
    // If we still have to carry an increment, then _next is now First() (which
    // we already emitted explicitly above)... so we're back to First() and
    // there are no new products to emit.
    SetNoMoreProducts();
  }

  return _next;
}

} /* namespace protobag */
