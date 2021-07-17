//g++  7.4.0
#include <bits/stdc++.h>
using namespace std;
#define no_of_user_pages 50000
#define no_of_kernel_pages 10000
#define no_of_blocks 10000
#define threeWayMemoryType 3
#define twoWayMemoryType 2
#define block_size 100
#define MAX_ORDER 12
#define kernel_blocks 100

/*
threeWayMemoryType=0 is for kernel space
threeWayMemoryType=1 is for user space
threeWayMemoryType=2 is for hybrid pages
*/

class page_block
{
	int start_ind;
	int page_block_order;
	int migrate_type;
    int memory_type;
	int free_pages;
	public:
	void init_page_block(int start,int order,int type,int pages,int memoryType) {
		start_ind=start;
	    page_block_order=order;
	    migrate_type=type;
        memory_type=memoryType;
	    free_pages=(1<<order)-pages;
	}
    
    int getPageOrder(page_block &pb) {
        return pb.page_block_order;
    }
    int getStartInd(page_block &pb) {
        return pb.start_ind;
    }
    int getMigrateType(page_block &pb) {
        return pb.migrate_type;
    }
    int getFreePages(page_block &pb) {
        return pb.free_pages;
    }
    int getMemoryType(page_block &pb) {
        return pb.memory_type;
    }
};

class ThreeWayClassification
{
    vector<pair<int, int>> free_memory[threeWayMemoryType][MAX_ORDER+1];//type 0 is for kernal and type 1 for user space
    map<int,page_block> mp;//stores page number for the particular application
    unordered_map<int,int> inUse[twoWayMemoryType];//maps the start address to the length of memmory in use
    int unmovableBlock[twoWayMemoryType][no_of_user_pages/(1<<MAX_ORDER) + 1]; //non zero value means block is unmovable
    int movableBlock[twoWayMemoryType][no_of_user_pages/(1<<MAX_ORDER) + 1]; //non zero value means block is unmovable    
    
    public:
    ThreeWayClassification()
    {
    	// Maximum number of powers of 2 possible
	    for(int i = 0; i <= MAX_ORDER; i++) {
	    	free_memory[0][i].clear();
            free_memory[1][i].clear();
            free_memory[2][i].clear();
		}
        
	    // Initially whole block of specified
	    // size is available
	    for(int i=0;(i+(1<<MAX_ORDER) - 1) < no_of_user_pages;i+=(1<<MAX_ORDER)) {
	    	free_memory[1][MAX_ORDER].push_back(make_pair(i, i + (1<<MAX_ORDER) - 1 ));
		}
        for(int i=0;(i+(1<<MAX_ORDER) - 1) < no_of_kernel_pages;i+=(1<<MAX_ORDER)) {
            free_memory[0][MAX_ORDER].push_back(make_pair(i, i + (1<<MAX_ORDER) - 1 ));
		}
	    
        //initially all blocks are movable
        int size=no_of_user_pages/(1<<MAX_ORDER) + 1;
        for(int i=0;i<size;i++) {
            unmovableBlock[0][i]=0;
            unmovableBlock[1][i]=0;

            movableBlock[0][i]=0;
            movableBlock[1][i]=0;
        }
    }
    
    void markBlock(int migrate_type,int memory,int memory_type) {
        int large_page_no=memory/(1<<MAX_ORDER);
        if(!migrate_type) {
            unmovableBlock[memory_type][large_page_no]++;
        }else {
            movableBlock[memory_type][large_page_no]++;
        }
    }
    void unmarkBlock(int migrate_type,int memory,int memory_type) {
        int large_page_no=memory/(1<<MAX_ORDER);
        if(!migrate_type) {
            unmovableBlock[memory_type][large_page_no]--;
        }else {
            movableBlock[memory_type][large_page_no]--;
        }
    }

    int CoalescingBlocks(int start,int page_order,int memory_type) {
        
        // Add the block in free list
        free_memory[memory_type][page_order].push_back(make_pair(start , start + (1<<page_order) - 1)); 
        
        /*cout << "Memory block from " << start 
             << " to "<< start + (1<<page_order) - 1
             << " freed\n";*/
        
        int i, buddyNumber, buddyAddress,highest_order=page_order;
        // Search in free list to find it's buddy
        for(int order=page_order;order<MAX_ORDER;order++) {
            // Calculate buddy number
            buddyNumber = start/(1<<order); 

            if (buddyNumber % 2 != 0) {
                buddyAddress = start - (1<<order);
            } else {
                buddyAddress = start + (1<<order);
            }
            
            highest_order=order;
            bool Coalesce=false;

            for(i = 0; i < free_memory[memory_type][order].size(); i++) 
            {

                // If buddy found and is also free
                if (free_memory[memory_type][order][i].first == buddyAddress) 
                {
                    // Now merge the buddies to make 
                    // them one large free memory block
                    if (buddyNumber % 2 == 0)
                    {
                        free_memory[memory_type][order + 1].push_back(make_pair(start,
                           start + 2 * (1<<order) - 1));

                        //cout << "Coalescing of blocks starting at " << start << " and " << buddyAddress << " was done" << "\n";
                    }
                    else
                    {
                        free_memory[memory_type][order + 1].push_back(make_pair(
                            buddyAddress, buddyAddress +
                            2 * (1<<order) - 1));

                        //cout << "Coalescing of blocks starting at " << buddyAddress << " and " << start << " was done" << "\n";
                        start=buddyAddress;
                    }
                    free_memory[memory_type][order].erase(free_memory[memory_type][order].begin() + i);
                    free_memory[memory_type][order].erase(free_memory[memory_type][order].begin() + free_memory[memory_type][order].size() - 1);
                    Coalesce=true;
                    break;
                }
            }
            if(!Coalesce) {
                break;
            }
        }
        return highest_order;
    }
    
    int smallPageCompaction() {
        int buddyNumber, buddyAddress,MaxOrderPage=-1;
        for(auto it=mp.begin(); it!=mp.end() ; it++) {
            int id=it->first;
            int order = it->second.getPageOrder(it->second); 
            int migrate_type=it->second.getMigrateType(it->second);
            int memory_type=it->second.getMemoryType(it->second);
            int start=it->second.getStartInd(it->second);
            int pages=(1<<order) - it->second.getFreePages(it->second);
            
            int buddy = start/(1<<order),buddyAdd; 
            if (buddy % 2 != 0) {
                buddyAdd = start - (1<<order);
            } else {
                buddyAdd = start + (1<<order);
            }
            int large_page_no=start/(1<<MAX_ORDER);
            if(!migrate_type || inUse[memory_type].find(buddyAdd)!=inUse[memory_type].end()) {
                continue;
            }
            //now the migrate_type==1
            for(int j=0; j < free_memory[memory_type][order].size(); j++) {
                int startAdd=free_memory[memory_type][order][j].first;
                
                buddyNumber = startAdd/(1<<order); 

                if (buddyNumber % 2 != 0) {
                    buddyAddress = startAdd - (1<<order);
                } else {
                    buddyAddress = startAdd + (1<<order);
                }
                
                if(startAdd!=buddyAdd && inUse[memory_type].find(buddyAddress)!=inUse[memory_type].end()) {
                    page_block pblock;
                    pblock.init_page_block(startAdd,order,migrate_type,pages,memory_type);
                    mp[id]=pblock;
                    inUse[memory_type][startAdd]= 1<<order;
                    
                    free_memory[memory_type][order].erase(free_memory[memory_type][order].begin() + j);
                    MaxOrderPage=max(MaxOrderPage,CoalescingBlocks(start,order,memory_type));
                    inUse[memory_type].erase(start);
                    break;
                }
            }
        }
        return MaxOrderPage;
    }
    
    int compaction() {
        int buddyNumber, buddyAddress,MaxOrderPage=-1;
        for(auto it=mp.begin(); it!=mp.end() ; it++) {
            int id=it->first;
            int order = it->second.getPageOrder(it->second); 
            int migrate_type=it->second.getMigrateType(it->second);
            int memory_type=it->second.getMemoryType(it->second);
            int start=it->second.getStartInd(it->second);
            int pages=(1<<order) - it->second.getFreePages(it->second);
            
            int buddy = start/(1<<order),buddyAdd; 
            if (buddy % 2 != 0) {
                buddyAdd = start - (1<<order);
            } else {
                buddyAdd = start + (1<<order);
            }
            int large_page_no=start/(1<<MAX_ORDER);
            //(movableBlock[memory_type][large_page_no] && unmovableBlock[memory_type][large_page_no])
            if(!migrate_type || inUse[memory_type].find(buddyAdd)!=inUse[memory_type].end() || (movableBlock[memory_type][large_page_no] && unmovableBlock[memory_type][large_page_no])) {
                continue;
            }
            //now the migrate_type==1
            for(int j=0; j < free_memory[memory_type][order].size(); j++) {
                int startAdd=free_memory[memory_type][order][j].first;
                int large_page_no1=startAdd/(1<<MAX_ORDER);
                
                if(movableBlock[memory_type][large_page_no1] && unmovableBlock[memory_type][large_page_no1]) {
                    continue;
                }

                buddyNumber = startAdd/(1<<order); 

                if (buddyNumber % 2 != 0) {
                    buddyAddress = startAdd - (1<<order);
                } else {
                    buddyAddress = startAdd + (1<<order);
                }
                
                if(startAdd!=buddyAdd && inUse[memory_type].find(buddyAddress)!=inUse[memory_type].end()) {
                    page_block pblock;
                    pblock.init_page_block(startAdd,order,migrate_type,pages,memory_type);
                    mp[id]=pblock;
                    inUse[memory_type][startAdd]= 1<<order;
                    unmarkBlock(migrate_type,start,memory_type);
                    markBlock(migrate_type,startAdd,memory_type);

                    free_memory[memory_type][order].erase(free_memory[memory_type][order].begin() + j);
                    MaxOrderPage=max(MaxOrderPage,CoalescingBlocks(start,order,memory_type));
                    inUse[memory_type].erase(start);
                    break;
                }
            }
        }
        return MaxOrderPage;
    }
    
    pair<int,int> getBlock(int page_order,int memory_type,pair<int,int> temp1) {
        
        int order=temp1.first,ind=temp1.second;
	    pair<int,int> temp = free_memory[memory_type][order][ind];
	  
	    // Remove first block to split
        // it into halves
	    free_memory[memory_type][order].erase(free_memory[memory_type][order].begin() + ind); 
        order--;
	    
	    for(;order >= page_order; order--) {          
            // Divide block into two halves
	        pair<int, int> pair1, pair2; 
	        pair1 = make_pair(temp.first, temp.first + (temp.second - temp.first) / 2);
	        pair2 = make_pair(temp.first + (temp.second - temp.first + 1) / 2, temp.second);                
            
            // Push them in free list               
	        free_memory[memory_type][order].push_back(pair1);
	        free_memory[memory_type][order].push_back(pair2);
            
            int size=free_memory[memory_type][order].size();
	        temp = free_memory[memory_type][order][size-1];
            
            // Remove first free block to 
	        // further split
            free_memory[memory_type][order].pop_back(); 
        }
        //cout << "Memory from " << temp.first << " to " << temp.second << " allocated" << "\n";
        return temp;
    }

    pair<int, int> findHybridBlock(int page_order,int memory_type) {
        // If not, search for a larger block
        int order,ind;
        pair<int, int> temp;
        temp={MAX_ORDER+1,MAX_ORDER+1};
        for(order = page_order ; order < MAX_ORDER+1; order++) {             
            // Find block size greater 
	       // than request
           if (free_memory[memory_type][order].size() != 0) {
               int j=0,n=free_memory[memory_type][order].size();
               for(;j<n;j++) {
                   int large_page_no=free_memory[memory_type][order][j].first/(1<<MAX_ORDER);
                   if(movableBlock[memory_type][large_page_no] && unmovableBlock[memory_type][large_page_no]) {
                       break;
                   }
               }
               if(j!=n) {
                   ind=j;
                   break;
               }
           }
	    }
        if(order==MAX_ORDER+1) {
            return temp;
        }
        temp={order,ind};
        return temp;
    }

    pair<int, int> findBlock(int page_order,int memory_type) {
        int order,ind;
        pair<int, int> temp;
        temp={MAX_ORDER+1,MAX_ORDER+1};
        for(order = page_order ; order < MAX_ORDER+1; order++) {             
            // Find block size greater 
	       // than request
           if (free_memory[memory_type][order].size() != 0) {
               int j=0,n=free_memory[memory_type][order].size();
               for(;j<n;j++) {
                   int large_page_no=free_memory[memory_type][order][j].first/(1<<MAX_ORDER);
                   if(page_order<MAX_ORDER || !movableBlock[memory_type][large_page_no] || !unmovableBlock[memory_type][large_page_no]) {
                       break;
                   }
               }
               if(j!=n) {
                   ind=j;
                   break;
               }
           }
	    }
        if(order==MAX_ORDER+1) {
            return temp;
        }
        temp={order,ind};
        return temp;
    }
    
    bool allocateMemory(int id,int pages,int migrate_type) {
        static int i=0;
        i++;
        // Calculate index in free list 
	    // to search for block if available
	    int page_order = ceil((double)log2(pages)); 
	    page_block pblock;
	    // Block available
        pair<int,int> order=findBlock(page_order,migrate_type);
            
	    // If no such block is found 
	    // i.e., no memory block available
        //cerr<<1<<":"<<order.first<<":"<<order.second<<" ";
	    if (order.first == MAX_ORDER+1) 
	    {
            int Max;
            
            if(page_order==MAX_ORDER) {
                Max=compaction();
            } else {
                Max=smallPageCompaction();
            }
            //cerr<<2<<":"<<Max<<" ";
            order=findBlock(page_order,migrate_type);
            if(migrate_type && order.second>=page_order && order.second!=MAX_ORDER+1) {
                //cerr<<3<<":"<<order.first<<":"<<order.second<<" ";
                
                pair<int,int> temp=getBlock(page_order,migrate_type,order);  
                pblock.init_page_block(temp.first,page_order,migrate_type,pages,migrate_type);
                inUse[migrate_type][temp.first] = 1<<page_order;
                markBlock(migrate_type,temp.first,migrate_type);
            } else {
                
                order=findHybridBlock(page_order,migrate_type);
                //cerr<<4<<":"<<order.first<<":"<<order.second<<" ";
                //try to steal page block from another type of memory
                if(order.first!=MAX_ORDER+1) {
                    pair<int,int> temp=getBlock(page_order,migrate_type,order);  
                    pblock.init_page_block(temp.first,page_order,migrate_type,pages,migrate_type);
                    inUse[migrate_type][temp.first] = 1<<page_order;
                    markBlock(migrate_type,temp.first,migrate_type);
                    mp[id] = pblock;
                    return true;
                }
                
                order=findHybridBlock(page_order,!migrate_type);
                //cerr<<5<<":"<<order.first<<":"<<order.second<<" ";
                //try to steal page block from another type of memory
                if(order.first!=MAX_ORDER+1) {
                    pair<int,int> temp=getBlock(page_order,!migrate_type,order);  
                    pblock.init_page_block(temp.first,page_order,migrate_type,pages,!migrate_type);
                    inUse[!migrate_type][temp.first] = 1<<page_order;
                    markBlock(migrate_type,temp.first,!migrate_type);
                    mp[id] = pblock;
                    return true;
                }
                
                order=findBlock(page_order,!migrate_type);
                //cerr<<6<<":"<<order.first<<":"<<order.second<<" ";
                //try to steal page block from another type of memory
                if(order.first!=MAX_ORDER+1) {
                    pair<int,int> temp=getBlock(page_order,!migrate_type,order);  
                    pblock.init_page_block(temp.first,page_order,migrate_type,pages,!migrate_type);
                    inUse[!migrate_type][temp.first] = 1<<page_order;
                    markBlock(migrate_type,temp.first,!migrate_type);
                } else {
                    //cout << "Sorry, failed to allocate memory\n";
                    return false;
                }
            }
        } else {
            pair<int,int> temp=getBlock(page_order,migrate_type,order);  
            pblock.init_page_block(temp.first,page_order,migrate_type,pages,migrate_type);
            inUse[migrate_type][temp.first] = 1<<page_order;
            markBlock(migrate_type,temp.first,migrate_type);
        }
	          
	    // If found
        mp[id] = pblock;
        return true;
    }
    
    void deallocateMemory(int id) {

        // If no such starting address available
        if(mp.find(id) == mp.end()) {
            //cout << "Sorry, invalid free request\n";
            return;
        }

        // Size of block to be searched
        int page_order = mp[id].getPageOrder(mp[id]); 
        int migrate_type=mp[id].getMigrateType(mp[id]);
        int memory_type=mp[id].getMemoryType(mp[id]);
        int start=mp[id].getStartInd(mp[id]);

        unmarkBlock(migrate_type,start,memory_type);
        CoalescingBlocks(start,page_order,memory_type);

        // Remove the key existence from map
        inUse[memory_type].erase(start);
        mp.erase(id); 
    }
};

int main()
{
    //srand(time(0));
    ThreeWayClassification mem;

    int n=2000000,m=(1<<MAX_ORDER),i;
    int user_success=0,user_tot=0,kernel_success=0,kernel_tot=0,itr=0,rpt=100;
    vector<int> temp;
    for(i=0;i<n;i++) {
        double x=((double)rand()/INT_MAX);
        double x1=((double)rand()/INT_MAX); 
        //cout<<x1<<" ";
        int y=(x1> (double)0.35)? 1:0;
        int type=(((double)rand()/INT_MAX) > 0.55)? 1:0;
        //fout << y << " ";
        if(y) 
        {
            if(itr==rpt-1) {
                x=1.0;
            }
            int memory=(int)(x*m);

            if(memory==0) {
                memory=m;
            }
            if(mem.allocateMemory(i,memory,type)) {
                if(memory>(m/2))
                {
                    if(type) {
                        user_success++;
                    }
                    else {
                        kernel_success++;
                    }
                }
                temp.push_back(i);
            }
            //fout << i<<" "<<memory<<" "<< type << "\n";
            //cout << i<<" "<<memory<<" "<< type << "\n";
            if(memory>(m/2))
                {
                    if(type) {
                        user_tot++;
                    }
                    else {
                        kernel_tot++;
                    }
                }
            itr++;
            itr%=rpt;
        }
        else {
            if(temp.size()==0) {
                mem.deallocateMemory(i);
                //fout << i <<" "<< INT_MAX <<" "<< INT_MAX << "\n";
            }
            else {
                int size=temp.size();
                int ind=size*((double)rand()/INT_MAX);
                mem.deallocateMemory(temp[ind]);
                temp.erase(temp.begin() + ind);
                //fout << temp[ind] <<" "<< INT_MAX <<" "<< INT_MAX << "\n";
            }
        }
    }
    cout<<"No. of User large space request: "<<user_tot<<"\n"<<"No. of User large space request fullfilled: "<<user_success<<"\n";
    cout<<"No. of Kernel large space request: "<<kernel_tot<<"\n"<<"No. of Kernel large space request fullfilled: "<<kernel_success;

}
//gcc ThreeWayFinal.cpp -o thf
//g++ -g -Wall ThreeWayFinal.cpp -o thf
