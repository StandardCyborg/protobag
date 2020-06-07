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

};

inline const MaybeProduct &IterProducts::GetNext() {
  // Return EndOfSequence forever or init
  if (NoMoreProducts()) {
    return MaybeProduct::EndOfSequence();
  } else if (_next.IsEndOfSequence()) {
    _next = MaybeProduct::First(NumPools());
    return _next;
  }

  // Compute next
  bool carry = true;
  for (size_t p = 0; p < NumPools(); ++p) {
    if (carry) {
      { _next.indices[p] += 1; carry = false; }
      if (_next.indices[p] == _pool_sizes[p]) {
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
    // Then we got back to First; there is no next
    SetNoMoreProducts();
  }

  return _next;
}

} /* namespace protobag */
