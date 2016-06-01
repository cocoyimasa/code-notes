#ifndef _COLLECTION_H
#define _COLLECTION_H
#include <iostream>
#include "Common.h"

class Object
{
public:
	virtual ~Object(){}
};

class NotCopyable{
private:
	NotCopyable(const NotCopyable&){}
	NotCopyable& operator=(const NotCopyable&){ return *this; }
public:
	NotCopyable(){}
};

class Interface :private NotCopyable{
public:
	virtual ~Interface()
	{}
};
///=================基础设施 Basic facilities / Foundation=========================
/// Plain-Old-Data Type 基本数据类型 返回true表示是基本类型
/// 此处可以充分看出所谓模板元编程其实相当于if...else，或者说模式匹配，此所谓类型计算
/// 读垃圾代码读多了果然深受其害！此时读回轮子哥精致的代码方恍然大悟！
template<typename T>
struct POD
{
	/// <summary>Returns true if the type is a Plain-Old-Data type.</summary>
	static const bool Result = false;
};

template<>struct POD<bool>{ static const bool Result = true; };
template<>struct POD<signed __int8>{ static const bool Result = true; };
template<>struct POD<unsigned __int8>{ static const bool Result = true; };
template<>struct POD<signed __int16>{ static const bool Result = true; };
template<>struct POD<unsigned __int16>{ static const bool Result = true; };
template<>struct POD<signed __int32>{ static const bool Result = true; };
template<>struct POD<unsigned __int32>{ static const bool Result = true; };
template<>struct POD<signed __int64>{ static const bool Result = true; };
template<>struct POD<unsigned __int64>{ static const bool Result = true; };
template<>struct POD<char>{ static const bool Result = true; };
template<>struct POD<wchar_t>{ static const bool Result = true; };
template<typename T>struct POD<T*>{ static const bool Result = true; };
template<typename T>struct POD<T&>{ static const bool Result = true; };
template<typename T, typename C>struct POD<T C::*>{ static const bool Result = true; };
template<typename T, int _Size>struct POD<T[_Size]>{ static const bool Result = POD<T>::Result; };
template<typename T>struct POD<const T>{ static const bool Result = POD<T>::Result; };
template<typename T>struct POD<volatile T>{ static const bool Result = POD<T>::Result; };
template<typename T>struct POD<const volatile T>{ static const bool Result = POD<T>::Result; };

///类型计算
///移除类型引用以及类型修饰词
template<typename T>
struct RemoveReference
{
	typedef T			Type;
};

template<typename T>
struct RemoveReference<T&>
{
	typedef T			Type;
};

template<typename T>
struct RemoveReference<T&&>
{
	typedef T			Type;
};

template<typename T>
struct RemoveConst
{
	typedef T			Type;
};

template<typename T>
struct RemoveConst<const T>
{
	typedef T			Type;
};

template<typename T>
struct RemoveVolatile
{
	typedef T			Type;
};

template<typename T>
struct RemoveVolatile<volatile T>
{
	typedef T			Type;
};

template<typename T>
struct RemoveCVR
{
	typedef T								Type;
};

template<typename T>
struct RemoveCVR<T&>
{
	typedef typename RemoveCVR<T>::Type		Type;
};

template<typename T>
struct RemoveCVR<T&&>
{
	typedef typename RemoveCVR<T>::Type		Type;
};

template<typename T>
struct RemoveCVR<const T>
{
	typedef typename RemoveCVR<T>::Type		Type;
};

template<typename T>
struct RemoveCVR<volatile T>
{
	typedef typename RemoveCVR<T>::Type		Type;
};
///int&&可以理解为右值引用
template<typename T>
typename RemoveReference<T>::Type&& MoveValue(T&& value)
{
	return (typename RemoveReference<T>::Type&&)value;
}
///================================
template<typename T>
struct KeyType {
public:
	typedef T Type;

	static T GetKeyValue(const T& value){
		return value;
	}
};
/// Why virtual inherits?
/// Because it's a interface,allow multiple inheritance
template<typename T>
class IEnumerator : public virtual Interface{
	///可枚举类型的基本操作：Clone,Current,Index,Next,Reset,Evalated
public:
	typedef T ElementType;

	virtual IEnumerator<T>* Clone()const = 0;
	virtual const T& Current()const = 0;
	virtual int Index() const = 0;
	virtual bool Next()=0;
	virtual void Reset()=0;
	virtual bool Evalated()const{ return false; };
};

template<typename T> 
class IEnumerable : public virtual Interface{
public:
	typedef T ElementType;
	virtual IEnumerator<T>* CreateEnumerator()const = 0;

};

template<typename T,bool PODType>
class ListStore abstract : public Object{

};

template<typename T>
class ListStore<T, false> : public Object{
protected:
	static void CopyObjects(T* dest, const T* source, int count){
		///防止内存重叠
		/// dst...........src 如果dst在前，先复制前边，防止前边被覆盖
		/// src...........dst 如果dst在后，先复制后边，防止后边被覆盖
		if (dest < src)
		{
			for (int i = 0; i < count; i++){
				dest[i] = MoveValue(source[i]);
			}
		}
		else if (dest > src)
		{
			for (int i = count - 1; i >= 0; i++){
				dest[i] = MoveValue(source[i]);
			}
		}
		//else equal , no need to copy
	}
	static void ClearObjects(T* dest, int count){
		for (int i = 0; i < count; i++){
			dest[i] = T();
		}
	}
};
template<typename T>
class ListStore < T, true > abstract:public Object{
protected:
	static void CopyObjects(T* dest, const T* source, int count){
		memmove(dest, source, sizeof(T)*count);
	}
	static void ClearObjects(T* dest, int count){
		
	}
};
template<typename T>
class ArrayBase abstract : public ListStore<T,POD<T>::Result>, public virtual IEnumerable<T>{
protected:
	class Enumerator : public Object, public virtual IEnumerator < T > {
	protected:
		const ArrayBase<T>* container;
		int index;
	public:
		Enumerator(const ArrayBase<T>* _container, int _index = -1){
			container = _container;
			index = _index;
		}
		IEnumerator<T>* Clone()const{
			return new Enumerator(container,index);
		}
		const T& Current()const{
			return container->Get(index);
		}
		int Index() const{
			return index;
		}
		bool Next(){
			index++;
			return index>=0 && index < container->Count();
		}
		void Reset(){
			index = -1;
		}

	};
	T* buffer;
	int count;
public:
	ArrayBase()
		:buffer(0), count(0)
	{}
	///CreateEnumerator,Count,Get,operator[]
	IEnumerator<T>* CreateEnumerator()
	{
		return;
	}
	int Count()const{
		return count;
	}
	const T& Get(int index){
		CHECK_ERROR(index >= 0 && index < count, L"ArrayBase::Get(int) - array index out of range");
		return buffer[index];
	}
	T& operator[](int index){
		CHECK_ERROR(index >= 0 && index < count, L"ArrayBase::operator[](int) - array index out of range");
		return buffer[index];
	}
};
/// Why two types needed?
template<typename T,typename K = typename KeyType<T>::Type>
class ListBase abstract : public ArrayBase <T>
{
protected:
	int capacity;
	bool lessMemoryMode;
	int CalculateCapacity(int expected)
	{
		int result = capacity;
		if (expected > capacity){
			result = capacity / 4 * 5 + 1;
		}
		return result;
	}
	void MakeRoom(int index, int _count){
		int newCount = ArrayBase<T>::count + _count;
		if (newCount > capacity){
			int newCapacity = CalculateCapacity(newCount);
			T* newBuffer = new T[newCapacity];
			ListStore<T, POD<T>::Result>::CopyObjects(newBuffer, ArrayBase<T>::buffer, index);
			ListStore<T, POD<T>::Result>::CopyObjects(newBuffer + index + _count, 
													  ArrayBase<T>::buffer+index, 
													  ArrayBase<T>::count-index);
			delete[] ArrayBase<T>::buffer;
			ArrayBase<T>::buffer = newBuffer;
			capacity = newCapacity;
		}
		else
		{
			ListStore<T, POD<T>::Result>::CopyObjects(ArrayBase<T>::buffer+index+_count, 
													  ArrayBase<T>::buffer, 
													  ArrayBase<T>::count-index);
		}
		ArrayBase<T>::count = newCount;
	}
	void ReleaseUnnecessaryBuffer(int previousCount){
		
	}
public:
	ListBase(){
		ArrayBase<T>::count = 0;
		ArrayBase<T>::buffer = 0;
		capacity = 0;
		lessMemoryMode = true;
	}
	~ListBase()
	{
		delete[] ArrayBase<T>::buffer;
	}
	void SetLessMemoryMode(bool mode){
		lessMemoryMode = mode;
	}
	bool RemoveAt(int index){
		CHECK_ERROR(index >= 0 && index < this->Count(), L"ListBase<T,K>::removeAt(int)-List index out of range");
		int previousCount = ArrayBase<T>::count;
		ListStore<T, POD<T>::Result>::CopyObjects(ArrayBase<T>::buffer+index,
												  ArrayBase<T>::buffer+index+1,
												  ArrayBase<T>::count-index-1
												  );
		ArrayBase<T>::count--;
		//
		return true;
	}
	bool RemoveRange(int index, int _count){
		CHECK_ERROR(index>=0 && index<ArrayBase<T>::count,L"ListBase<T,K>::removeRange(int,int)-List index out of range")
		CHECK_ERROR(index+_count >= 0 && index + _count < ArrayBase<T>::count, L"ListBase<T,K>::removeRange(int,int)-index+_count out of range");
		int previousCount = ArrayBase<T>::count;
		ListStore<T, POD<T>::Result>::CopyObjects(ArrayBase<T>::buffer + index,
												  ArrayBase<T>::buffer + index + _count,
												  ArrayBase<T>::count - index -_count
												  );
		ArrayBase<T>::count -= _count;
		//
		return true;
	}
	bool Clear(){
		capacity = 0;
		if (lessMemoryMode){
			ListStore<T, POD<T>::Result>::Clear(ArrayBase<T>::buffer,
												ArrayBase<T>::count);

		}
		else
		{

		}
	}
};


#endif