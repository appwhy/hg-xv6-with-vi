#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
   char c;
   setShowAtOnce(0);
   setBufferFlag(0);
   while(1){
	read(0,&c,1);
	printf(1,"%d  %d %c\n",(unsigned char)c,c,c);
	if(c=='q')
		break;
   }
   setShowAtOnce(1);
   setBufferFlag(1);
  exit();
}
