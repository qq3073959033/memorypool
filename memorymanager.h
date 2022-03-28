#pragma once
#include <stdlib.h>
#include <iostream>
#include <assert.h>
#include <mutex>
#include <functional>
#include <unordered_map>
using namespace std;
class MemoryPool;
// 内存块信息，放在真实可用内存的前面
class MemoryBlock
{
public:
	int nId_;				     // 方便调试,内存块的编号
	int nRef_;				     // 内存块的引用技术，调试
	bool bPool_;				 // 是否属于内存池
	MemoryPool* pPool_;		     // 我们要创建多个不同的大小的内存池，标记内存块所属的内存池。
	MemoryBlock* pNext_;         // 组成链表结构
};

class MemoryPool
{
public:
	MemoryPool(size_t blockSize, size_t blockCount)
		: pPool_(nullptr)
		, blockSize_(blockSize)
		, blockCount_(blockCount)
		, freeBlock_(nullptr)
		, memoryLen_((sizeof(MemoryBlock) + blockSize_) * blockCount_)
	{
		initMemory();
	}

	// 初始化内存池
	void initMemory()
	{
		if (pPool_ != nullptr)
		{
			return;
		}
		pPool_ = (char*)malloc((blockSize_ + sizeof(MemoryBlock)) * blockCount_);
		if (pPool_ == nullptr)
		{
			std::cout << "the memory is empty" << std::endl;
			return;
		}
		MemoryBlock* pBlock = (MemoryBlock*)pPool_;
		freeBlock_ = pBlock;
		MemoryBlock* lastBlock = (MemoryBlock*)((char*)pPool_ + memoryLen_);
		int i = 0;
		for (; pBlock < lastBlock;)
		{
			pBlock->bPool_ = true;
			pBlock->nId_ = i++;
			pBlock->nRef_ = 0;
			pBlock->pPool_ = this;
			pBlock->pNext_ = (MemoryBlock*)(((char*)pBlock) + (blockSize_ + sizeof(MemoryBlock)));
			pBlock = (MemoryBlock*)(((char*)pBlock) + (blockSize_ + sizeof(MemoryBlock)));
		}
		pBlock = (MemoryBlock*)(((char*)pBlock) - (blockSize_ + sizeof(MemoryBlock)));
		pBlock->pNext_ = nullptr;
	}

	void* getMemoryBlock(size_t size)
	{
		// TODO: 加锁
		lock_guard<mutex> lock(mtx_);
		if (freeBlock_ == nullptr)
		{
			MemoryBlock* pBlock = (MemoryBlock*)malloc(blockSize_ + sizeof(MemoryBlock));
			if (pBlock == nullptr)
			{
				throw "the memory is empty";
			}
			pBlock->nId_ = -1;
			pBlock->nRef_ = 1;
			pBlock->pPool_ = nullptr;
			pBlock->pNext_ = nullptr;
			pBlock->bPool_ = false;
			// 返回给用户实际可用的地址
			return pBlock + 1;
		}
		else
		{
			MemoryBlock* freeBlock = freeBlock_;
			freeBlock_->nRef_++;
			freeBlock_ = freeBlock_->pNext_;
			return freeBlock + 1;
		}
	}

	void test()
	{
		MemoryBlock* pBlock = (MemoryBlock*)pPool_;
		for (int i = 0; i < blockCount_; ++i)
		{
			cout << (((char*)pBlock) + (blockSize_ + sizeof(MemoryBlock))) - (char*)pBlock << endl;
			pBlock = (MemoryBlock*)(((char*)pBlock) + (blockSize_ + sizeof(MemoryBlock)));
		}
	}

	void rebackMemoryBlock(MemoryBlock* pBlcok)
	{
		lock_guard<mutex> lock(mtx_);
		pBlcok->pNext_ = freeBlock_;
		freeBlock_ = pBlcok;
	}
private:
	size_t blockSize_;       // 内存池每一个块的大小
	size_t blockCount_;		 // 内存池存储多少个块节点
	char* pPool_;            // 内存池申请内存的指针
	MemoryBlock* freeBlock_; // 指向下一个空闲的内存块
	mutex mtx_;              // 线程安全操作
	size_t memoryLen_;       // 总字节数
};

template<typename size_t blockSize, typename size_t blockCount>
class InitPool : public MemoryPool
{
public:
	InitPool()
		:MemoryPool(blockSize, blockCount)
	{

	}
};



// 内存管理工具
// 实现为单例类，并不需要它有多个实例
// 抽象出这个管理类的作用：将申请内存的逻辑交给这个类去处理
// 不要把大量的逻辑写在new里头
class MemoryManager
{
public:
	~MemoryManager()
	{

	}

	static MemoryManager& instance()
	{
		static MemoryManager manger;
		return manger;
	}

	void* AllocMemory(size_t size)
	{
		if (size <= 64)
		{
			return pool1_.getMemoryBlock(size);
		}
		else if (64 < size <= 128)
		{
			return pool2_.getMemoryBlock(size);
		}
		else if (128 < size <= 256)
		{
			return pool3_.getMemoryBlock(size);
		}
		else if (256 < size <= 512)
		{
			return pool4_.getMemoryBlock(size);
		}
		return pool1_.getMemoryBlock(size);
	}

	void FreeMemory(void* ptr)
	{
		// 获得实际申请内存的地址空间
		MemoryBlock* pBlock = ((MemoryBlock*)ptr) - 1;
		assert(--(pBlock->nRef_) == 0);
		if (pBlock->bPool_ == true)
		{
			pBlock->pPool_->rebackMemoryBlock(pBlock);
		}
		else
		{
			free(pBlock);
		}
	}

	void test()
	{
		pool1_.test();
	}
private:
	MemoryManager()
	{
	}
	InitPool<64, 1000000> pool1_;
	InitPool<128, 100> pool2_;
	InitPool<258, 100> pool3_;
	InitPool<512, 100> pool4_;
};