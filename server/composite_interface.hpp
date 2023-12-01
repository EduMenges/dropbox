#pragma once

namespace dropbox {

class CompositeInterface {
   public:
    virtual ~CompositeInterface() = default;
    virtual void Remove(int id)   = 0;
};

}
