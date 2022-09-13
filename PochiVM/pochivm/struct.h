
#pragma once
#include <numeric>
#include "api_base.h"
#include "api_vt_base.h"

class Object {
public:
    using fieldType = std::map<std::string, PochiVM::TypeId>;
    using offsetType = std::map<std::string, size_t>;
    Object(fieldType fields) :
      mFields(fields),
      mOffsets(getOffsets()),
      mSize(std::accumulate(mOffsets.begin(), mOffsets.end(), 0UL, [](auto a, auto b){return a + b.second;})),
      mObjectPtr(PochiVM::CallFreeFn::pochi_malloc(
            PochiVM::Literal<unsigned long>(mSize)))
       {

       }
    operator PochiVM::Value<void *>() {
        return mObjectPtr;
    }
    PochiVM::ReferenceVT operator[](std::string field) {
        unsigned long offset = mOffsets.at(field);
        PochiVM::TypeId type = mFields.at(field);
        return *PochiVM::ReinterpretCast(type.AddPointer(), PochiVM::ReinterpretCast<char *>(mObjectPtr) + offset);
    }
    // only works for multiples who are powers of 2
    // https://stackoverflow.com/questions/3407012/rounding-up-to-the-nearest-multiple-of-a-number
    static unsigned long roundup(unsigned long numToRound, unsigned long multiple) 
    {
        return (numToRound + multiple - 1) & -multiple;
    }
    offsetType getOffsets() {
        offsetType result;
        size_t size = 0;
        for(auto field : mFields) {
            result[field.first] = size;
            size += roundup(field.second.Size(), sizeof(double));
        }
        return result;
    }
private:
    const fieldType mFields;
    const offsetType mOffsets;
    const size_t mSize;
    const PochiVM::Value<void *> mObjectPtr;
};
