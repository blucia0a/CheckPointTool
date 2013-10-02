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
#include <sys/resource.h>
#include <assert.h>
#include <map>

/*The size of the stack to copy when taking a checkpoint*/
unsigned long CPSTACK_SIZE;
/*The buffer to copy to when taking a checkpoint*/
char *cpStack;
/*The context object that will store the register state, etc 
  when taking a checkpoint*/
CONTEXT currentCheckpoint;
/*A variable that tells our checkpointing function whether
  to capture a new checkpoint or to resume from the existing
  one*/
bool resumingFromCheckPoint = false;

INT32 usage()
{
    cerr << "-=CheckPointTool=- A tool for adding checkpoint/restart to C programs, using Pin";
    cerr << KNOB_BASE::StringKnobSummary();
    cerr << endl;
    return -1;
}


VOID CaptureCheckPoint(CONTEXT *ctx){

  if(resumingFromCheckPoint == true){

    /*The way the instrumentation works out, 
      when we resume from a checkpoint, we
      end up passing through the instrumentation
      that takes a checkpoint. At this point, we've
      set all the registers back to the values
      in the stored context.  That means we can
      go and refill the stack from our stored copy.  
      Of course when resuming it is not useful to
      take another checkpoint, so we just return.
    */
    ADDRINT sp = PIN_GetContextReg(ctx, REG_STACK_PTR);
    memcpy((void*)sp, (void*)cpStack, CPSTACK_SIZE);

    return;

  }

  fprintf(stderr,"Taking a checkpoint!\n");
  /*Get the stack pointer, copy the contents of the 
    stack to cpStack, our checkpoint buffer*/
  ADDRINT sp = PIN_GetContextReg(ctx, REG_STACK_PTR);
  memcpy(cpStack, (void*)sp, CPSTACK_SIZE);
  PIN_SaveContext(ctx, &currentCheckpoint);

}

VOID RestoreCheckPoint(){

  /*ExecuteAt restores the register state.
    resumingFromCheckPoint = true makes the 
    checkpoint capturing code (that we will
    jump to after ExecuteAt) restore the stack*/
  resumingFromCheckPoint = true;
  PIN_ExecuteAt(&currentCheckpoint);

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

  cpStack = (char *)calloc(CPSTACK_SIZE,1);
  //struct rlimit rv;
  //getrlimit(RLIMIT_STACK,&rv);
  //fprintf(stderr,"Max Stack Size = %lu\n",(unsigned long)rv.rlim_cur);
  CPSTACK_SIZE = 4096;//(unsigned long)rv.rlim_cur - 1;
  TRACE_AddInstrumentFunction(instrumentTrace, 0);

  PIN_InterceptSignal(SIGTERM,termHandler,0);
  PIN_InterceptSignal(SIGSEGV,segvHandler,0);

  PIN_AddThreadStartFunction(threadBegin, 0);
  PIN_AddThreadFiniFunction(threadEnd, 0);
  PIN_AddFiniFunction(Fini, 0);
    
 
  PIN_StartProgram();
  
  return 0;
}
