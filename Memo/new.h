#ifndef MEMO__NEW_H__INCLUDED
#define MEMO__NEW_H__INCLUDED

void* operator new(std::size_t n) throw(std::bad_alloc);

void oprator delete(void* p) throw();

#endif  // MEMO__NEW_H__INCLUDED

