/**
# Copyright (C) 2012-2014 Chincisan Octavian-Marius(udfjj39546284@gmail.com) - getic.net - N/A
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
*/

#ifndef TINYCLASSES_H
#define TINYCLASSES_H

#include <os.h>
#include <stdlib.h>
#include <vector>

template <typename U> class CntPtr
{
public:
    explicit CntPtr(U* p = 0) : _c(0) {if (p) _c = new cnt(p);}
    ~CntPtr(){dec();}
    CntPtr(const CntPtr& r) throw(){add(r._c);}
    CntPtr& operator=(const CntPtr& r){if (this != &r){dec(); add(r._c);} return *this;}
    U& operator*()  const throw()   {return *_c->p;}
    U* operator->() const throw()   {return _c->p;}
private:
    struct cnt {
        cnt(U* p = 0, unsigned int c = 1) : p(p), c(c) {}
        U*          p;
        unsigned int  c;
    }* _c;
    void add(cnt* c) throw(){_c = c;if (c) ++c->c;}
    void dec(){ if (_c) {if (--_c->c == 0) {delete _c->p; delete _c;}_c = 0;}}
};


template <class T> class DPool // dynamic pool gets from heap when out of objs
{
public:
    class U : public T
    {
    public:
        U():_polled(0){};
        virtual ~U(){};
        int     _polled;
    };
public:
	static void create_pool(unsigned int cap)
	{
        _cs = new mutex();

		_pvect = new std::vector<U*>();
		_pvect->reserve(cap);
		_pbhead = (U*) ::malloc(cap * sizeof(U));
        _nCapacity = cap;
		U* pw	= _pbhead;
		for(unsigned int i=0; i< cap; i++){
			_pvect->push_back(pw++);
		}
	}

	static void destroy_pool(){
		if(0==_pvect)return;
		_pvect->clear();
		delete _pvect;
		::free((void*)_pbhead);
		_pbhead=0;
        delete _cs;
	}

	void* operator new(size_t sz)
	{
		if(T::_pvect->size() > 0)
        {
            AutoLock q(_cs);
			int szisz = _pvect->size();
			U* pvb = (U*)_pvect->back();
			_pvect->pop_back();
			++DPool<T>::_inUse;
            ((U*)pvb)->_polled = szisz;
			return (void*)(pvb);
		}
        else
        {
            U* pu = ::new U();
            pu->_polled=-1;
            return pu;
        }
	}

	void operator delete(void* pv){
        if( ((U*)pv)->_polled !=-1)
        {
            AutoLock q(_cs);
            --DPool<T>::_inUse;
		    _pvect->push_back((U*)pv);
        }
        else
            delete pv;
	}
    static int capacity(){
        return _nCapacity;
    }
    static int elements(){
        return _inUse;
    }
	static U*			 	 _pbhead;
	static std::vector<U*>*	 _pvect;
	static unsigned int     _nCapacity;
	static unsigned int	    _inUse;
    static mutex            *_cs;
};

template <class T> typename DPool<T>::U*            DPool<T>::_pbhead;
template <class T> std::vector<typename DPool<T>::U*>*   DPool<T>::_pvect;
template <class T> unsigned int				                DPool<T>::_nCapacity;
template <class T> unsigned int				                DPool<T>::_inUse;
template <class T> mutex*  		                    DPool<T>::_cs;

template <class T, size_t SZ = 64>
class  Bucket
{
public:
	Bucket():_elems(0){}
	~Bucket(){}
	T& operator[](size_t idx){return t[idx];}
	const T& operator[](size_t idx)const{return t[idx];}
    bool push(T el){if(_elems < SZ) {t[_elems++] = el; return true;}; return false;}
    T*  pop(){ if(_elems) {--_elems; return &t[_elems];} return 0;}
	void remove(size_t idx){if(_elems){t[idx] = t[_elems-1];t[_elems-1]=0;_elems--;}}
    void clear(){_elems=0;}
    size_t size()const {return _elems;}
    T* begin(){return t;}
    T* at(size_t n){return &t[n];}
    T* end(){return &t[_elems];} //this should point to 0
protected:
	T	   t[SZ];
	size_t _elems;
};
#endif // TINYCLASSES_H
