//g++  7.4.0

#include <bits/stdc++.h>
using namespace std;
#define no_of_pages 2048
#define no_of_blocks 10000
#define twoWayMemoryType 2
#define block_size 100
#define MAX_ORDER 11
#define kernel_blocks 100

/*
twoWayMemoryType=0 is for kernel space
twoWayMemoryType=1 is for user space
*/

class page_block
{
	int start_ind;
	int page_block_order;
	int migrate_type;
	int free_pages;
	public:
	void init_page_block(int start,int order,int type,int pages) {
		start_ind=start;
	    page_block_order=order;
	    migrate_type=type;
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
};


class TwoWayClassification
{
    vector<pair<int, int>> free_memory[twoWayMemoryType][MAX_ORDER+1];//type 0 is for kernal and type 1 for user space
    map<int,page_block> mp;//stores page number for the particular application
    map<int,int> inUse[twoWayMemoryType];//maps the start address to the length of memmory in use
    
    public:
    TwoWayClassification()
    {
    	// Maximum number of powers of 2 possible
	    for(int i = 0; i <= MAX_ORDER; i++) {
	    	free_memory[0][i].clear();
            free_memory[1][i].clear();
		}
	        
	  
	    // Initially whole block of specified
	    // size is available
	    for(int i=0;(i+(1<<MAX_ORDER) - 1) < no_of_pages;i+=(1<<MAX_ORDER)) {
	    	free_memory[0][MAX_ORDER].push_back(make_pair(i, i + (1<<MAX_ORDER) - 1 ));
            free_memory[1][MAX_ORDER].push_back(make_pair(i, i + (1<<MAX_ORDER) - 1 ));
		}
	    
    }
    
        
    int CoalescingBlocks(int start,int page_order,int migrate_type) {
        
        // Add the block in free list
        free_memory[migrate_type][page_order].push_back(make_pair(start , start + (1<<page_order) - 1)); 
        
        cout << "Memory block from " << start 
             << " to "<< start + (1<<page_order) - 1
             << " freed\n";
        
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

            for(i = 0; i < free_memory[migrate_type][order].size(); i++) 
            {

                // If buddy found and is also free
                if (free_memory[migrate_type][order][i].first == buddyAddress) 
                {
                    // Now merge the buddies to make 
                    // them one large free memory block
                    if (buddyNumber % 2 == 0)
                    {
                        free_memory[migrate_type][order + 1].push_back(make_pair(start,
                           start + 2 * (1<<order) - 1));

                        cout << "Coalescing of blocks starting at "
                             << start << " and " << buddyAddress
                             << " was done" << "\n";
                    }
                    else
                    {
                        free_memory[migrate_type][order + 1].push_back(make_pair(
                            buddyAddress, buddyAddress +
                            2 * (1<<order) - 1));

                        cout << "Coalescing of blocks starting at "
                             << buddyAddress << " and "
                             << start << " was done" << "\n";
                        start=buddyAddress;
                    }
                    free_memory[migrate_type][order].erase(free_memory[migrate_type][order].begin() + i);
                    free_memory[migrate_type][order].erase(free_memory[migrate_type][order].begin() + free_memory[migrate_type][order].size() - 1);
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
    
    int compaction() {
        int buddyNumber, buddyAddress,MaxOrderPage=-1;
        for(auto it=mp.begin(); it!=mp.end() ; it++) {
            int id=it->first;
            int order = it->second.getPageOrder(it->second); 
            int migrate_type=it->second.getMigrateType(it->second);
            int start=it->second.getStartInd(it->second);
            int pages=(1<<order) - it->second.getFreePages(it->second);
            
            int buddy = start/(1<<order),buddyAdd; 
            if (buddy % 2 != 0) {
                buddyAdd = start - (1<<order);
            } else {
                buddyAdd = start + (1<<order);
            }
            
            if(!migrate_type || inUse[migrate_type].find(buddyAdd)!=inUse[migrate_type].end()) {
                continue;
            }
            
            for(int j=0; j < free_memory[migrate_type][order].size(); j++) {
                int startAdd=free_memory[migrate_type][order][j].first;
                
                buddyNumber = startAdd/(1<<order); 

                if (buddyNumber % 2 != 0) {
                    buddyAddress = startAdd - (1<<order);
                } else {
                    buddyAddress = startAdd + (1<<order);
                }
                
                if(startAdd!=buddyAdd && inUse[migrate_type].find(buddyAddress)!=inUse[migrate_type].end()) {
                    page_block pblock;
                    pblock.init_page_block(startAdd,order,migrate_type,pages);
                    mp[id]=pblock;
                    inUse[migrate_type][startAdd]= 1<<order;
                    
                    free_memory[migrate_type][order].erase(free_memory[migrate_type][order].begin() + j);
                    MaxOrderPage=max(MaxOrderPage,CoalescingBlocks(start,order,migrate_type));
                    inUse[migrate_type].erase(start);
                    break;
                }
            }
        }
        //cout<<MaxOrderPage<<" ";
        return MaxOrderPage;
    }
    
    int findBlock(int page_order,int migrate_type) {
        // If not, search for a larger block
        int i;
        for(i = page_order + 1; i < MAX_ORDER+1; i++) {             
            // Find block size greater 
	       // than request
           if (free_memory[migrate_type][i].size() != 0) 
               break;
	    }
        return i; 
    }
    
    bool allocateMemory(int id,int pages,int migrate_type) {
        // Calculate index in free list 
	    // to search for block if available
	    int page_order = ceil((double)log2(pages)); 
	    
	    // Block available
	    if (free_memory[migrate_type][page_order].size() > 0) 
	    {
	        pair<int, int> temp = free_memory[migrate_type][page_order][0]; 
	  
	        // Remove block from free list
	        free_memory[migrate_type][page_order].erase(free_memory[migrate_type][page_order].begin()); 
	          
	        cout << "Memory from " << temp.first
	             << " to " << temp.second 
	             << " allocated" << "\n";
              
	        // Map starting address with 
	        // size to make deallocating easy
	        page_block pblock;
	        pblock.init_page_block(temp.first,page_order,migrate_type,pages);
            inUse[migrate_type][temp.first]=1<<page_order;
	        mp[id] = pblock; 
            return true;
	    }
	    else
	    {
	        int i=findBlock(page_order,migrate_type);
	  
	        // If no such block is found 
	        // i.e., no memory block available
	        if (i == MAX_ORDER+1) 
	        {
                i=compaction();
                if(i>=page_order) {
                    i=findBlock(page_order,migrate_type);
                } else {
                    cout << "Sorry, failed to allocate memory\n";
                    return false;
                } 
	        }
	          
	        // If found
	        pair<int, int> temp;
	        temp = free_memory[migrate_type][i][0];
	  	
	        // Remove first block to split
            // it into halves
	        free_memory[migrate_type][i].erase(free_memory[migrate_type][i].begin()); 
	        i--;
	              
	        for(;i >= page_order; i--) 
	        {          
                // Divide block into two halves
	            pair<int, int> pair1, pair2; 
	            pair1 = make_pair(temp.first, temp.first + (temp.second - temp.first) / 2);
	            pair2 = make_pair(temp.first + (temp.second - temp.first + 1) / 2, temp.second);
	                
                // Push them in free list               
	            free_memory[migrate_type][i].push_back(pair1);
	            free_memory[migrate_type][i].push_back(pair2);
					 
	            temp = free_memory[migrate_type][i][0];
	  
                // Remove first free block to 
	            // further split
	            free_memory[migrate_type][i].erase(free_memory[migrate_type][i].begin()); 
            }
	              
	        cout << "Memory from " << temp.first << " to " << temp.second << " allocate" << "\n";

		    page_block pblock;
		    pblock.init_page_block(temp.first,page_order,migrate_type,pages);
            inUse[migrate_type][temp.first] = 1<<page_order;
            mp[id] = pblock;
	    }
        return true;
    }
    
    void deallocateMemory(int id) {

        // If no such starting address available
        if(mp.find(id) == mp.end()) 
        {
            cout << "Sorry, invalid free request\n";
            return;
        }

        // Size of block to be searched
        int page_order = mp[id].getPageOrder(mp[id]); 
        int migrate_type=mp[id].getMigrateType(mp[id]);
        int start=mp[id].getStartInd(mp[id]);

        CoalescingBlocks(start,page_order,migrate_type);
        
        // Remove the key existence from map
        inUse[migrate_type].erase(start);
        mp.erase(id); 
    }
};

int main()
{
    TwoWayClassification mem;
    mem.allocateMemory(1,512,1); 
    mem.allocateMemory(2,512,1);
    mem.allocateMemory(3,512,1);
    mem.allocateMemory(4,512,1);
    mem.deallocateMemory(3);
    mem.deallocateMemory(2);
    mem.allocateMemory(4,1024,1);
}
