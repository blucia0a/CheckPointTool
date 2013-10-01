#include "pin.H"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>
#include <string.h>
#include <dlfcn.h>
#include <sys/time.h>
#include <assert.h>
#include <map>


map<unsigned long, std::pair<unsigned long, char *> > stackMap;
ADDRINT lastMemAddr;

CONTEXT currentCheckpoint;
map<unsigned long, std::pair<unsigned long, char *> > checkpointMap;

INT32 usage()
{
    cerr << "-=HarvSim=- An interruption-prone device simulator";
    cerr << KNOB_BASE::StringKnobSummary();
    cerr << endl;
    return -1;
}


VOID LogStackOp(){

  if( stackMap.find(lastMemAddr) != stackMap.end() ){

    std::pair<unsigned long, char *> existing = stackMap[ lastMemAddr ];
    memcpy((void*)(existing.second), (void*)lastMemAddr, (existing.first));

  }else{

    assert(false && "Last memory access was not set up properly\n");

  }
  lastMemAddr = 0;

}

VOID SetupStackOp(ADDRINT memAddr, UINT32 accSize){

  if( lastMemAddr != 0 ){
    LogStackOp();
  }

  lastMemAddr = memAddr;
  std::pair<unsigned long, char *> existing;
  char *buf = NULL;
  if( stackMap.find(memAddr) != stackMap.end() ){
    /*Found the entry.  It has been written before*/

    existing = stackMap[ memAddr ];
    if( accSize == existing.first ){

      /*Easy: same size as existing entry*/
      return;

    }else{

      /*hard: different size from existing entry*/
      /*need to free that, then do what we normally do*/
      free( existing.second );

    }

  }

  buf = (char *)calloc(1,accSize);
  
  existing.second = buf;
  
  existing.first = accSize;
  
  stackMap[ memAddr ] = existing;
  
  return;

}

void copyStackMapToCheckPointMap(){

  map<unsigned long, std::pair<unsigned long, char *> >::iterator sMapIter, sMapEnd;
  for(sMapIter = stackMap.begin(), sMapEnd = stackMap.end(); sMapIter != sMapEnd; sMapIter++){
    unsigned long addr = sMapIter->first;
    checkpointMap[ addr ] = sMapIter->second;
  }

}

VOID CaptureCheckPoint(CONTEXT *ctx){

  if( lastMemAddr != 0 ){
    LogStackOp();
  }

  fprintf(stderr,"Taking a checkpoint!\n");  
  copyStackMapToCheckPointMap();
  PIN_SaveContext(ctx, &currentCheckpoint);

}

VOID RestoreCheckPoint(){

  if( lastMemAddr != 0 ){
    LogStackOp();
  }
  
  map<unsigned long, std::pair<unsigned long, char *> >::iterator sMapIter, sMapEnd;
  for(sMapIter = checkpointMap.begin(), sMapEnd = checkpointMap.end(); sMapIter != sMapEnd; sMapIter++){

    unsigned long addr = sMapIter->first;
    memcpy((void*)addr, (void*)(sMapIter->second.second), sMapIter->second.first);

  }

  fprintf(stderr,"Restoring a checkpoint!\n");  
  PIN_ExecuteAt(&currentCheckpoint);

}


VOID instrumentRoutine(RTN rtn, VOID *v){
    
  if(strstr(RTN_Name(rtn).c_str(),"CHECKPOINT_NOW")){
    RTN_Open(rtn);
    RTN_InsertCall(rtn, 
                   IPOINT_AFTER, 
                   (AFUNPTR)CaptureCheckPoint, 
                   IARG_CONTEXT,
                   IARG_END);
    RTN_Close(rtn);
  }
  
  if(strstr(RTN_Name(rtn).c_str(),"RESTORE_NOW")){
    RTN_Open(rtn);
    RTN_InsertCall(rtn, 
                   IPOINT_BEFORE, 
                   (AFUNPTR)RestoreCheckPoint, 
                   IARG_END);
    RTN_Close(rtn);
  }
   
}


VOID instrumentImage(IMG img, VOID *v)
{

}


VOID instrumentTrace(TRACE trace, VOID *v)
{

  for( BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl) ){

    for(INS ins = BBL_InsHead(bbl); INS_Valid(ins); ins = INS_Next(ins) ){

      if( INS_IsStackWrite(ins) ){

        INS_InsertCall(ins, IPOINT_BEFORE,(AFUNPTR)SetupStackOp, 
                       IARG_MEMORYWRITE_EA, IARG_MEMORYWRITE_SIZE, 
                       IARG_END);

        if( INS_HasFallThrough( ins ) ){
          /*This gets most.  Any not gotten this way, are gotten by
            the if(lastMemAddr == 0) guard in Setup... Checkpoint... 
            and Restore...
          */
          INS_InsertCall(ins, IPOINT_AFTER,(AFUNPTR)LogStackOp,IARG_END);
        }

      }

      if( INS_IsDirectCall(ins) && INS_IsProcedureCall(ins) ){

        ADDRINT add = INS_DirectBranchOrCallTargetAddress(ins);

        RTN rtn = RTN_FindByAddress(add);

        if( RTN_Valid(rtn) ){

          if(strstr(RTN_Name(rtn).c_str(),"RESTORE_NOW")){
            INS_InsertCall(ins, IPOINT_BEFORE,(AFUNPTR)RestoreCheckPoint,IARG_END);
          }

          if(strstr(RTN_Name(rtn).c_str(),"CHECKPOINT_NOW")){
            INS_InsertCall(ins, IPOINT_BEFORE,(AFUNPTR)CaptureCheckPoint,IARG_CONTEXT,IARG_END);
          }

        }

      }

    }

  }

}


VOID threadBegin(THREADID threadid, CONTEXT *sp, INT32 flags, VOID *v)
{
  
}
    
VOID threadEnd(THREADID threadid, const CONTEXT *sp, INT32 flags, VOID *v)
{

}

VOID dumpInfo(){
}


VOID Fini(INT32 code, VOID *v)
{
 
  
}

BOOL segvHandler(THREADID threadid,INT32 sig,CONTEXT *ctx,BOOL hasHndlr,const EXCEPTION_INFO *pExceptInfo, VOID*v){
  return TRUE;//let the program's handler run too
}

BOOL termHandler(THREADID threadid,INT32 sig,CONTEXT *ctx,BOOL hasHndlr,const EXCEPTION_INFO *pExceptInfo, VOID*v){
  return TRUE;//let the program's handler run too
}


int main(int argc, char *argv[])
{

  PIN_InitSymbols();
  if( PIN_Init(argc,argv) ) {
    return usage();
  }

  stackMap.clear();

  //RTN_AddInstrumentFunction(instrumentRoutine,0);
  //IMG_AddInstrumentFunction(instrumentImage, 0);
  TRACE_AddInstrumentFunction(instrumentTrace, 0);

  PIN_InterceptSignal(SIGTERM,termHandler,0);
  PIN_InterceptSignal(SIGSEGV,segvHandler,0);

  PIN_AddThreadStartFunction(threadBegin, 0);
  PIN_AddThreadFiniFunction(threadEnd, 0);
  PIN_AddFiniFunction(Fini, 0);
    
 
  PIN_StartProgram();
  
  return 0;
}
