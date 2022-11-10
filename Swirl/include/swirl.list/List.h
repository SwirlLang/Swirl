#include <vector>

#ifndef SWIRL_LIST_H
#define SWIRL_LIST_H

class SwirlList {
    std::vector<void*> m_ItemPointers;
    std::vector<std::size_t> m_ItemSize;
public:
    template < class _Type >
    void newItem(_Type);
    void deleteItem(std::size_t);
    auto getElement(std::size_t);
};

#endif
