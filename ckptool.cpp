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


CONTEXT currentCheckpoint;

INT32 usage()
{
    cerr << "-=HarvSim=- An interruption-prone device simulator";
    cerr << KNOB_BASE::StringKnobSummary();
    cerr << endl;
    return -1;
}

VOID CaptureCheckPoint(CONTEXT *ctx){

  fprintf(stderr,"Taking a checkpoint!\n");  
  PIN_SaveContext(ctx, &currentCheckpoint);

}

VOID RestoreCheckPoint(){

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
