#include "bootpack.h"

unsigned int memtest(unsigned int start,unsigned int end)
{
	char flag486 = 0;
	unsigned int eflag,cr0,i;
	//确认是386还是486
	eflag = io_load_eflags();
	eflag |= EFLAGS_AC_BIT;				//设定AC_BIT = 1,386设完后会自动归零
	io_store_eflags(eflag);
	eflag = io_load_eflags();
	if((eflag & EFLAGS_AC_BIT) != 0)	//判断AC_BIT是否为1
	{
		flag486 = 1;
	}
	eflag &= ~EFLAGS_AC_BIT;			//AC_BIT = 0
	io_store_eflags(eflag);
	
	if(flag486 != 0)
	{
		cr0 = load_cr0();
		cr0 |= CR0_CACHE_DISABLE;		//禁止缓存
		store_cr0(cr0);
	}
	
	i = memtest_sub(start,end);
	
	if(flag486 != 0)
	{
		cr0 = load_cr0();
		cr0 &= ~CR0_CACHE_DISABLE;		//启用缓存
		store_cr0(cr0);
	}
	return i;
}

void memman_init(struct MEMMAN *man)
{
	man->frees = 0;						//可用信息条数
	man->maxfrees = 0;					//用于观察可用状况：frees的最大值
	man->lostsize = 0;					//释放失败的总空间大小
	man->losts = 0;						//释放失败的次数
	return;
}

unsigned int memman_total(struct MEMMAN *man)		//报告可用内存总空间
{
	unsigned int i,t =0;
	for(i = 0;i < man->frees;i++)
		t += man->free[i].size;
	return t;
}

unsigned int memman_alloc(struct MEMMAN *man,unsigned int size)		//分配内存
{
	unsigned int i,a;
	for(i = 0;i < man->frees;i++)
	{
		if(man->free[i].size >= size)				//找到了足够大的内存空间
		{
			a = man->free[i].addr;					//找出地址
			man->free[i].addr += size;				//将空间从可用空间里除去
			man->free[i].size -= size;				//同上
			if(man->free[i].size == 0)				//减掉不必要的可用信息
			{
				man->frees --;
				for(;i < man->frees;i++)
				{
					man->free[i] = man->free[i + 1];
				}
			}
			return a;
		}
	}
	return 0;										//没找到足够大的可用空间
}

int memman_free(struct MEMMAN *man,unsigned int addr,unsigned int size)
{
	int i,j;
	for(i = 0;i < man->frees;i++)					//找到被释放的地址在表中的位置
	{
		if(man->free[i].addr > addr)
			break;
	}
	if(i > 0)										//如果之前有剩余空间
	{
		if(man->free[i - 1].addr + man->free[i - 1].size == addr)	//可以与之前的空间合并
		{
			man->free[i - 1].size += size;
			if(i < man->frees)						//之后有剩余空间
			{
				if(addr + size == man->free[i].addr)	//可以与之后的剩余空间合并
				{
					man->free[i - 1].size += man->free[i].size;
					man->frees --;
					for(;i < man->frees;i++)			//合并后消除free[i]
					{
						man->free[i] = man->free[i + 1];
					}
				}
			}
			return 0;							//成功完成
		}
	}
	//至此：之前没有
	if(i < man->frees)						//如果之后有剩余空间
	{
		if(addr + size == man->free[i].addr)	//可合并
		{
			man->free[i].addr = addr;
			man->free[i].size += size;
			return 1;
		}
	}
	//至此：之前之后都没空间
	if(man->frees < MEMMAN_FREES)			//还可向表中加入
	{
		for(j = man->frees;j > i;j--)		//腾出空间
		{
			man->free[j] = man->free[j - 1];
		}
		//以下为加入新项
		man->frees ++;						
		if(man->maxfrees < man->frees)
		{
			man->maxfrees = man->frees;
		}
		man->free[i].addr = addr;
		man->free[i].size = size;
		return 2;
	}
	//至此:无法释放空间
	man->losts ++;
	man->lostsize += size;
	return -1;			//释放失败
}

unsigned int memman_alloc_4k(struct MEMMAN *man,unsigned int size)
{
	unsigned int a;
	size = (size + 0xfff) & 0xfffff000;		//以4k（0x1000字节）为单位向上取整
	a = memman_alloc(man,size);
	return a;
}

int memman_free_4k(struct MEMMAN *man,unsigned int addr,unsigned int size)
{
	int i;
	size = (size + 0xfff) & 0xfffff000;
	i = memman_free(man,addr,size);
	return i;
}