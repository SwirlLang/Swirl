#include <iostream>
#include <vector>
#include <memory>

#include <swirl.list/List.h>

template < class _Type >
void SwirlList::newItem(_Type _it) {
    m_ItemSize.push_back(sizeof(_it));
    void* i_alloca = malloc(sizeof(_it));
    m_ItemPointers.push_back(i_alloca);
    i_alloca = _it;
}

void SwirlList::deleteItem(std::size_t index) {
    m_ItemPointers.erase(m_ItemPointers.begin() + index);
    m_ItemSize.erase(m_ItemSize.begin() + index);
}

auto SwirlList::getElement(std::size_t index) {
    std::get_temporary_buffer<>()
}