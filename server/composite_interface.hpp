#pragma once

namespace dropbox {

/// Composite interface because C++ don't work well with cirular references.
class CompositeInterface {
   public:
    virtual ~CompositeInterface() = default;
    virtual void Remove(int id)   = 0;
};

}
