#include <stdlib.h>
#include <stdio.h>


void CHECKPOINT_NOW(){ fprintf(stderr,"<app>:In the ckpt func\n");}//<----(4)
void RESTORE_NOW(){ fprintf(stderr,"<app>:In the restore func\n");}

void bar(int qrs,int tuv){

  CHECKPOINT_NOW();
  fprintf(stderr,"Hi from bar! %x %x\n",qrs, tuv);//<----(5)

}

void foo(int qrs){

  fprintf(stderr,"Hi from foo! %x\n",qrs);//<----(2)
  bar(qrs,2300);

}

int i = 0;

int main(int argc, char *argv[]){

  fprintf(stderr,"<app>:Starting main!\n");//<----(1)

  foo(19); 
  
  fprintf(stderr,"<app>:took a checkpoint\n");//<----(6)
  
  fprintf(stderr,"<app>:At this point\n");//<----(7)

  if( i == 1 ){

    fprintf(stderr,"<app>:Outta here!!\n");
    exit(0);

  }


  i = 1;
  
  RESTORE_NOW();
  
  fprintf(stderr,"<app>:Shouldn't get here...\n");

}
