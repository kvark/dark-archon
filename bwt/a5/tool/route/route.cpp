#include <stdio.h>
#include <memory.h>
#include <assert.h>


enum{	ORDER = 3,
	SIZE = 1<<(ORDER+1)
};

template< typename Type >
class Swap	{
public:
	Swap(Type &a, Type &b)	{
		const Type c = a;
		a = b; b = c;
	}
};

typedef unsigned t_gid;

//	State declaration	//

class Group;

class State	{
	enum { MAX_SEQ = 50 };
	Group *seq[MAX_SEQ+1];
	Group *best[MAX_SEQ+1];
	t_gid mid[MAX_SEQ];
	int num,ptr[MAX_SEQ];
	unsigned timestamp;
	float volume,record;
public:
	enum CheckResult	{
		CR_CLEAN, CR_LOCKED, CR_SORTED
	};
	State();
	void reset();
	CheckResult check(const Group *const) const;
	void attach(Group *const);
	void detach(Group *const);
	bool add(const float);
	bool bad();
	void remember();
	void number(t_gid) const;
	void print(Group *const, const bool) const;
}static state;

//	Nodes definition	//

class NodeBase	{
	enum { MAX_SUB = 20 };
protected:
	NodeBase *sub[MAX_SUB+1];
	int num;
public:
	NodeBase(): num(0)	{
		memset(sub, 0, sizeof(sub));
	}
	void add(NodeBase *const nod)	{
		sub[num++] = nod;
		assert(num <= MAX_SUB);
	}
	virtual void count() = 0;
	virtual bool inc() = 0;
	virtual float ready() const = 0;
	virtual const char* get() const = 0;
	virtual ~NodeBase()	{}
};

class Dependance: public NodeBase	{
public:
	enum DepType {SUFFIX, PREFIX, PERIOD, SELF} const type;
	Dependance(const DepType type): type(type)	{
		//empty
	}
	Dependance(const Dependance &dep):type(SELF)	{
		memcpy(this, &dep, sizeof(Dependance));
	}
	void fill(Group *const, const int);
	virtual void count()	{
		for(int i=0; sub[i] /*&& !state.bad()*/; ++i)
			sub[i]->count();
	}
	virtual bool inc()	{
		int i;
		if(type == PREFIX) return false;
		for(i=0; sub[i] && !sub[i]->inc(); ++i);
		return sub[i] != NULL;
	}
	virtual float ready() const	{
		float sum = 0.f;
		for(int i=0; i!=num; ++i)
			sum += sub[i]->ready();
		return sum;
	}
	virtual const char* get() const	{
		static const char *st[SELF+1] = {
			"SUFFIX", "PREFIX", "PERIOD", "SELF"
		};
		return this ? st[type] : st[SELF];
	}
};

class Group: public	NodeBase	{
	int cur;
	unsigned lastamp;
	bool sorted;
	friend class State;
public:
	Group *parent;
	float volume;
	Group(): sorted(false),cur(-1), lastamp(0), parent(NULL), volume(0.f) {
		//empty
	}
	t_gid index() const	{
		const Group *pp = this;
		while(pp->parent) pp = pp->parent;
		return this+1 - pp;
	}
	virtual void count()	{
		//check if already counted
		switch(state.check(this))	{
			case State::CR_CLEAN:
				sorted = false;
				state.attach(this);
				if(cur < 0) state.add(volume);
				else sub[cur]->count();
				sorted = true;
				break;
			case State::CR_LOCKED:
				state.add(2.f);
			case State::CR_SORTED:
			default: break;
		}
	}
	virtual bool inc()	{
		const t_gid gid = index();
		if(state.check(this) != State::CR_CLEAN) return false;
		sorted = false;
		state.attach(this);
		if(cur<0 && state.add(volume) && !sub[++cur])	{
			state.add(-volume);
			state.detach(this);
			cur=-1; return false;
		}
		//switch current way
		if(cur<0 || !sub[cur]->inc()) do	{ ++cur;
			if(!sub[cur])	{
				state.detach(this);
				cur=-1; return false;
			}
			const float val = sub[cur]->ready();
			const bool rez = state.add(val);
			state.add(-val);
			if(!rez) break;
		}while(true);
		return (sorted=true);
	}
	virtual float ready() const	{
		const State::CheckResult cr = state.check(this);
		if(cr == State::CR_LOCKED) return 1.1f;
		if(cr == State::CR_SORTED) return 0.f;
		return volume;
	}
	virtual const char* get() const	{
		static const char str[] = "";
		return str;
	}
	virtual ~Group()	{
		for(int i=0; i!=num; ++i)
			delete sub[i];
	}
};

void Dependance::fill(Group *const gr, const int off)	{
	int i = off;
	assert(off < num);
	if(off+1 == num) gr->add(new Dependance(*this));
	else do	{
		for(int j=i; j!=num; ++j)	{
			Swap<NodeBase*>(sub[i], sub[j]);
			fill(gr,off+1);
			Swap<NodeBase*>(sub[i], sub[j]);
		}
	}while(++i != num-1);
}

//	State implementation	//

State::State(): num(-1),timestamp(0),record(1.5f)	{
	memset(best, 0, sizeof(best));
	memset(ptr, 0, sizeof(ptr));
}
void State::reset()	{
	num = 0;
	++timestamp;
	volume = 0.f;
}
State::CheckResult State::check(const Group *const nod) const	{
	for(const Group *par=nod; par; par = par->parent)	{
		if(par->lastamp == timestamp && par->sorted) return CR_SORTED;
	}
	if(nod->lastamp == timestamp) return CR_LOCKED;
	return CR_CLEAN;
}
void State::attach(Group *const nod)	{
	nod->lastamp = timestamp;
	seq[num++] = nod;
	mid[num-1] = nod+1 - seq[0];
	assert(num <= MAX_SEQ);
}
void State::detach(Group *const nod)	{
	assert(nod->lastamp == timestamp);
	assert(num>0);
	--num; --nod->lastamp;
	assert(seq[num] == nod);
	seq[num] = NULL;
	mid[num] = 0;
}
bool State::add(const float val)	{
	volume += val;
	return bad();
}
bool State::bad()	{
	return volume >= record;
}
void State::remember()	{
	if(bad()) return;
	if(num==6 && mid[0]==1 && mid[1]==2 && mid[2]==4 && mid[3]==5 && mid[4]==3 && mid[5]==6)	{
		int a = 0;
	}
	memcpy(best, seq, num*sizeof(Group*));
	best[num] = NULL;
	for(int i=0; i!=num; ++i)
		ptr[i] = seq[i]->cur;
	record = volume;
}
void State::number(t_gid id) const	{
	char str[20] = {0}, *ps = str+sizeof(str)-1;
	for(; id!=1; id>>=1)	{
		*--ps = '0' + (id & 1);
		assert(ps > str);
	}
	printf(".%-8s",ps);
}
void State::print(Group *const base, const bool local) const	{
	int limit = num;
	if(!local) for(limit=0; best[limit]; ++limit);
	for(int i=0; i!=limit; ++i)	{
		Group *const nod = local ? seq[i] : best[i];
		const int id = local ? nod->cur : ptr[i];
		number(nod-base);
		printf(" \tvol:%.3f \tway:%2d \t", nod->volume, id);
		if(id >= 0) printf("link type: %s", nod->sub[id]->get());
		printf("\n");
	}
	if(local) printf("- - - - - - - - - - - - - \n");
	printf("Total volume: %.3f\n", local ? volume : record);
}

//	Data construction part	//

static void fill_sizes(Group *const nod)	{
	t_gid i;
	nod[1].volume = 1.f;
	for(i=2; i!=SIZE; ++i)	{
		typedef int bit;
		int j,k,num;
		bit b[ORDER+1];
		float kf = 1.f;
		//get bit array
		for(j=0; (i>>j)!=1; ++j)
			b[ORDER-j] = (i>>j)&1;
		memcpy(b, b+ORDER+1-j, j*sizeof(bit));
		b[num=j] = 1;
		//count sum
		nod[i].volume = 0.f;
		for(k=0; k<=num; ++k)	{
			float cur = (kf /= k+1);
			int t = 1;
			if(!b[k]) continue;
			for(j=0; j!=k; ++j)
				cur *= 1 - 2*b[j];
			//get target
			for(j=k+1; j<num; ++j)
				t = t+t+b[j];
			cur *= nod[t].volume;
			nod[i].volume += cur;
		}
		/*
		for(j=0; j!=num; ++j)
			printf("%d",b[j]);
		printf(": %.2f\n",S[i]);
		*/
	}
}

static void fill_links(Group *const nod)	{
	for(t_gid i=1; i!=SIZE; ++i)	{
		nod[i].parent = (i==1 ? NULL : nod+(i>>1));
		//add suffix
		if(i+i < SIZE)	{	//not last line
			Dependance *pd = NULL;
			pd = new Dependance(Dependance::SUFFIX);
			pd->add(nod+i+i+0);
			pd->add(nod+i+i+1);
			nod[i].add(pd);
			pd = new Dependance(Dependance::SUFFIX);
			pd->add(nod+i+i+1);
			pd->add(nod+i+i+0);
			nod[i].add(pd);
		}
		//add prefix
		if(i!=1)	{	//not first line
			int j, mask;
			for(j=0; (i>>j)!=1; ++j);
			mask = (1<<--j)-1;
			assert(j>=0 && mask>=0);
			j = (i & mask) ^ (mask+1);
			Group *grup;
			for(grup=nod+i; grup!=NULL && grup!=nod+j; grup=grup->parent);
			if(!grup)	{
				Dependance *pd = new Dependance(Dependance::PREFIX);
				pd->add(nod + j);
				nod[i].add(pd);
			}
		}
		//add period
		if(i!=1 && i+i<SIZE)	{	//both
			t_gid mask,k; int j,l,e;
			Dependance dep(Dependance::PERIOD);
			
			for(j=0; (i>>j)!=1; ++j);
			mask = (2<<j)-1; k = i^(1<<j);
			k = (k<<j) + k + (1<<(j+j));

			for(l=0; l!=j; ++l)	{
				const t_gid gid = ((k>>l) & mask) ^ 1;
				for(e=0; e!=l; ++e)
					if(gid == (((k>>e) & mask) ^ 1)) break;
				if(e != l) continue;
				for(e=0; e!=j; ++e)
					if(gid == ((k>>e) & mask)) break;
				if(e != j) continue;
				dep.add(nod + (2<<j) + gid);
			}
			dep.fill(nod+i,0);
		}
	}
}

//	main program	//

int main()	{
	Group nod[SIZE];
	fill_sizes(nod);
	fill_links(nod);

	printf("Bruteforcing...");
	do	{
		state.reset();
		nod[1].count();
		state.print(nod,true);
		state.remember();
		state.reset();
	}while(nod[1].inc());
	printf("done.\n");
	state.print(nod,false);
	return 0;
}
