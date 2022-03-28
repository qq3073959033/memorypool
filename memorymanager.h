#pragma once
#include <stdlib.h>
#include <iostream>
#include <assert.h>
#include <mutex>
#include <functional>
#include <unordered_map>
using namespace std;
class MemoryPool;
// �ڴ����Ϣ��������ʵ�����ڴ��ǰ��
class MemoryBlock
{
public:
	int nId_;				     // �������,�ڴ��ı��
	int nRef_;				     // �ڴ������ü���������
	bool bPool_;				 // �Ƿ������ڴ��
	MemoryPool* pPool_;		     // ����Ҫ���������ͬ�Ĵ�С���ڴ�أ�����ڴ���������ڴ�ء�
	MemoryBlock* pNext_;         // �������ṹ
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

	// ��ʼ���ڴ��
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
		// TODO: ����
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
			// ���ظ��û�ʵ�ʿ��õĵ�ַ
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
	size_t blockSize_;       // �ڴ��ÿһ����Ĵ�С
	size_t blockCount_;		 // �ڴ�ش洢���ٸ���ڵ�
	char* pPool_;            // �ڴ�������ڴ��ָ��
	MemoryBlock* freeBlock_; // ָ����һ�����е��ڴ��
	mutex mtx_;              // �̰߳�ȫ����
	size_t memoryLen_;       // ���ֽ���
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



// �ڴ������
// ʵ��Ϊ�����࣬������Ҫ���ж��ʵ��
// ������������������ã��������ڴ���߼����������ȥ����
// ��Ҫ�Ѵ������߼�д��new��ͷ
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
		// ���ʵ�������ڴ�ĵ�ַ�ռ�
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