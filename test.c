#include <stdlib.h>
#include <stdio.h>


void CHECKPOINT_NOW(){ fprintf(stderr,"<app>:In the ckpt func\n");}
void RESTORE_NOW(){ fprintf(stderr,"<app>:In the restore func\n");}

void foo(int qrs){

  CHECKPOINT_NOW();
  fprintf(stderr,"Hi from foo! %d\n",qrs);

}


int main(int argc, char *argv[]){

  int i = 0;
  fprintf(stderr,"<app>:Starting main!\n");

  foo(19); 
  
  fprintf(stderr,"<app>:took a checkpoint\n");
  
  fprintf(stderr,"<app>:At this point\n");

  if( i == 1 ){
    fprintf(stderr,"<app>:Outta here!!\n");
    exit(0);
  }

  i = 1;
  
  RESTORE_NOW();
  
  fprintf(stderr,"<app>:Shouldn't get here...\n");
 

}
