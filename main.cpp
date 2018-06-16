#include <iostream>#include <iostream>
#include <fstream>
#include <math.h>
#define MAXSIZE 65536
#define FRAMESIZE 256
#define PAGESIZE 256
#define NUMBERofFRAMESinMAINMEMORY 256
#define NUMBERofELEMENTSinPAGETABLE 256
#define TLB_SIZE 16

using namespace std;

//  Note::Comments on an instruction follow it.
int main(){

    /**------------------SECTION 01: Read addresses---------------------**/
    //For each address : Offset is (address & 0xFF), Page Number is (address shifted 8 times to the right)
    int count=0 /*Addresses Counter*/ ,VirtualAddress[MAXSIZE] /*Address received here from addresses.txt*/, PageNum[MAXSIZE],Offset[MAXSIZE]; //Initializing the variables.
    std::ifstream infile("addresses.txt"); //Opening the file
    while(infile>>VirtualAddress[count]){ //Taking VirtualAddress addresses line by line
        Offset[count]=VirtualAddress[count] &0xFF; //masking the addresses to get the offset
        PageNum[count]=VirtualAddress[count]>>int(log2(double (FRAMESIZE)));
        //Shifting the address 8 bits to the right to obtain page number.
        //Generally, it is shifted n bits, where the size of the memory is 2^n bits.
     count++;
    }

   infile.clear();//Clear stream
   infile.close(); //Close file

   int PhysicalAddress[MAXSIZE];
    /**-------------------------------------------------------------------**/


    /**------------------SECTION 02: Define necessary structures ---------------------**/
    struct FRAME{char frame[FRAMESIZE];};
    struct PAGE {char page[PAGESIZE];}; //Unnecessary except for semantics.
    struct PAGETABLE_ELEMENT{int FRAME_NUMBER; bool REFERENCE_BIT=0; int USAGE_COUNTER; /*NEW::To be used in LRU Replacement Policy*/};
    //Page Table is an array of page table elements.
    // Reference bit (Valid/Invalid bit), set =0 for invalid initially, will be set to 1 when page is loaded in Main Memory.
    struct TLB_ELEMENT{int PAGE_NUMBER=-1; int FRAME_NUMBER;}; //NEW::TLB is an array of TLB elements.
    //Page Number is initially set to -1 to indicate that no page is present here.


    TLB_ELEMENT TLB[TLB_SIZE];

/*
    for(int w=0; w<TLB_SIZE;w++){       //Testing all TLB_ELEMENT::PAGE_NUMBER's are initially set to -1.
    cout<<w+1<<" : "<<TLB[w].PAGE_NUMBER<<endl;
    }
*/

    FRAME MAIN_MEMORY[NUMBERofFRAMESinMAINMEMORY]; //Defining Main Memory as a Contiguous array of Frames.
    PAGETABLE_ELEMENT PAGE_TABLE[NUMBERofELEMENTSinPAGETABLE];
    // Defining Page Table as a Contiguous array of PAGETABLE_ELEMENT's, where index is  Page Number.

    /*
    for(int w=0; w<NUMBERofELEMENTSinPAGETABLE;w++){       //Testing all REFERENCE_BIT's are initially set to 0.
    cout<< PAGE_TABLE[w].REFERENCE_BIT<<endl;
    }
    */

    /**------------------SECTION 03: Use TLB and Main Memory to read Data (if not TLB-HIT-> Main Memory, then load data if not there) ---------------------**/
    fstream myFile;
    myFile.open ("BACKING_STORE.bin", ios::in | ios::out | ios::binary);//Open the file (A handler).
    int Looper=count; //Set number of values to number of addresses.
    //Count has a maximum of 256*256 in our case. But it needn't be 256*256 since:
    //1. It is expected to be less than MAXSIZE, since usually not all memory locations are called.
    //2. Count can have different sizes depending on Our case, 256*256 here.
    int PAGE_FAULT=0; //To Calculate PAGE_FAULT rate.
    int TLB_HIT=0; //To Calculate TLB_HIT rate.
    bool IS_HIT=0; //Will be set to true only when a page is found in TLB, to prevent searching for it in PAGE_TABLE again.
    bool FOUND_EMPTY_TLB=0; // Set to true only when an element in TLB is empty (in the beginning 16 calls).
    int THE_FRAME_NUMBER=0; //The Frame number, first found either by Page Table or TLB.
    int min_USED_Page; //For LRU Replacement Policy.
    int min_USED_Page_IN=0; //Index of min_USED_Page in TLB
    char* value;
    value=new char[Looper]; //Dynamic character array for storing output values.


    int FRAME_COUNTER=0; //Increments by one each time a new frame is allocated to allow for allocation of next frame.
    for(int k=0; k<Looper;k++){ //Loop over all addresses to execute them (Model for Single Program).
            /**----------------------TLB-------------------------**/
            for(int TLB_LOOPER=0; TLB_LOOPER< TLB_SIZE; TLB_LOOPER++){
                if(TLB[TLB_LOOPER].PAGE_NUMBER == PageNum[k]){ /*i.e:: TLB-HIT*/
                    THE_FRAME_NUMBER= TLB[TLB_LOOPER].FRAME_NUMBER;
                    IS_HIT=1;
                    TLB_HIT++;
                }
            } //Loops on TLB to find the Page via its Page Number, if not found, IS_HIT will be 0;
            if(!IS_HIT){
                /**Determine Free TLB-Elements index, if not : Lowest Used Page's Index in TLB**/
                min_USED_Page= TLB[0].PAGE_NUMBER; //Default value, actual minimum will be determined via a loop.
                for(int TLB_LOOPER_0=0; TLB_LOOPER_0< TLB_SIZE; TLB_LOOPER_0++){
                    if(TLB[TLB_LOOPER_0].PAGE_NUMBER == -1){ //Find first Free place in TLB.
                    FOUND_EMPTY_TLB=1; //Set to true because we did find a free(EMPTY) TLB element.
                    min_USED_Page_IN= TLB_LOOPER_0; //Now min_USED_PAGE_IN points to a free TLB element, which is ok
                                                //since it will be replaced with the new page and that's the goal.
                    break;
                    }
                }
                if(!FOUND_EMPTY_TLB){ //Search for Least Used Page only if no free TLB element is found.
                for(int TLB_LOOPER=0; TLB_LOOPER< TLB_SIZE; TLB_LOOPER++){ //The Loop will determine min_USED_Page_IN;
                if(TLB[TLB_LOOPER].PAGE_NUMBER != -1){ //i.e. a NON-GARBAGE page was there.
                   if(PAGE_TABLE[TLB[TLB_LOOPER].PAGE_NUMBER].USAGE_COUNTER <= PAGE_TABLE[min_USED_Page].USAGE_COUNTER )
                   {min_USED_Page=TLB[TLB_LOOPER].PAGE_NUMBER;
                    min_USED_Page_IN=TLB_LOOPER;} //Determine INDEX OF TLB_ELEMENT with lowest used Page.
                }
                }
                }
                /**Replace Lowest Used Page with new Page**/
                /**Will be done after Page Table has returned the Page**/
                /**Find SIMILAR COMMENT below**/
            }

            /**--------------------END OF TLB-------------------------**/
            /**------------------BEGIN PAGE TABLE---------------------**/

            if(!IS_HIT && !PAGE_TABLE[PageNum[k]].REFERENCE_BIT ){ //i.e. if page for kth instruction is NOT valid(= not present in Main Memory)

                //Change the place of "get pointer" reading from file, set it = PageNumber of kth instruction * PAGESIZE
                //i.e. the get pointer is now on the

                PAGE_TABLE[PageNum[k]].FRAME_NUMBER=FRAME_COUNTER;
                myFile.seekg (PageNum[k]*PAGESIZE);
                myFile.read( MAIN_MEMORY[PAGE_TABLE[PageNum[k]].FRAME_NUMBER].frame/*loads page in FRAME_COUNTERth frame in MainMemory*/,
                                    PAGESIZE /*Number of read bytes starting from get pointer (i.e. Page Number* PAGESIZE)
                                                I set it to PAGESIZE to LOAD one PAGE at a time.*/);

                //Access Page Table using Page Number for kth instruction, set its frame number = current frame counter.
                //LOAD Page into Main Memory.

                PhysicalAddress[k]=FRAME_COUNTER*FRAMESIZE+Offset[k];
                THE_FRAME_NUMBER=PAGE_TABLE[PageNum[k]].FRAME_NUMBER;
                //Carry out Instruction from Main Memory (using Frame Number and Ofsset)
                PAGE_TABLE[PageNum[k]].REFERENCE_BIT=1; // Set Reference bit to valid.
                FRAME_COUNTER++;//Increment FRAME_COUNTER;
                PAGE_FAULT++;
        }else if(!IS_HIT && PAGE_TABLE[PageNum[k]].REFERENCE_BIT){ //If page for kth instruction is Loaded in Main Memory
        THE_FRAME_NUMBER=PAGE_TABLE[PageNum[k]].FRAME_NUMBER;
        //Access Page Table using Page Number for kth instruction, find the frame number for the page number, access the..
        //..Main Memory using the Frame Number, then access the data inside the frame using the offset for kth instruction.
        }
        /**------------------END OF PAGE TABLE---------------------**/

        /**-------------------The Missing Part in TLB----------------------**/
         /** SIMILAR COMMENT **/
         /**Replace Lowest Used Page with new Page**/
         /**Can be done now Since we are sure Page is in Main Memory and therefore has a frame number in Page Table**/
          if(!IS_HIT){
                TLB[min_USED_Page_IN].PAGE_NUMBER= PageNum[k];
                TLB[min_USED_Page_IN].FRAME_NUMBER= PAGE_TABLE[PageNum[k]].FRAME_NUMBER;
          }
          /**Replaced**/
        /**--------------------Done with Missing Part in TLB--------------**/
        PAGE_TABLE[PageNum[k]].USAGE_COUNTER++; //Increment Page Usage Counter.
        value[k]=MAIN_MEMORY[THE_FRAME_NUMBER].frame[Offset[k]]; //Get Value after Everything is settled.
        PhysicalAddress[k]=PAGE_TABLE[PageNum[k]].FRAME_NUMBER*FRAMESIZE+Offset[k];
        IS_HIT=0; //IMPORTANT !!! :: Set IS_HIT back to zero after each instruction is executed.
        FOUND_EMPTY_TLB=0; //IMPORTANT !!! :: Set back to zero to check again if an Empty element is found.

    }
    myFile.clear();//Clear stream.
    myFile.close();/**Equivalent to -> Close the Program.**/ //Close Stream.

    /**--------------------------------------------------------------------------------------------------------**/

    /**------------------------SECTION 04: Write Data to output File in Expected Format------------------------**/

    std::ofstream out("output.txt");
    for(int w=0; w<Looper;w++){ //Writing in given Output form.
        out/*<<w<<": Page Number: "<<PageNum[w]*/<<" Virtual Address: "<<VirtualAddress[w]/*<<" This Page was Used : "<<PAGE_TABLE[PageNum[w]].USAGE_COUNTER<<" times. "*/<<" Physical Address: "<<PhysicalAddress[w]<<" Value: "<<int(value[w])<<endl;
    }
    out<<"\n \n \n" <<"---------------------------------------------------------------------"<<endl;
    out<<"# of Page Faults is:: "<<PAGE_FAULT<<endl;
    out<<"# of TLB-Hits is:: "<< TLB_HIT<<endl;
    out<<"Page Fault rate is:: " << ((double)PAGE_FAULT/count)*100 <<endl;
    out<<"TLB-HIT rate is:: " << ((double)TLB_HIT/count)*100 <<endl;
    out<< "---------------------------------------------------------------------"<<endl;

    //Number of Page Faults. NB: Must be <= 256 in  non-Bonus case, since Main Memory can contain all
    //256 pages.
    out.clear(); //Clear stream.
    out.close(); //Close Stream.

    return 0;



}
    /**--------------------------------------------------------------------------------------------------------**/


