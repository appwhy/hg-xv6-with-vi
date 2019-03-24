/*
	2019-3-20
	vi_3 在 vi_2 的基础上基本上补全了所有的浏览命令，如下所示：
		G : 移到文件末尾
		g : 移到文件开头

		PgUp：向上翻页
		PgDn：向下翻页

		上下左右键（既可以用hjkl，也可以用方向键），Enter键向下移动一行，Space键向右移动一个字符。

		w ： 向右移动一个单词
		e ： 移动到下一个单词的尾字符
		b ： 移动到上一个单词的首字符

		H ： 将光标移到屏幕最上方一行
		M ： 将光标移到屏幕中间
		L ： 将光标移到屏幕最下方一行

	-------------------------------------------------------------------------------------------------------
	说明：正常情况下，若一行只有"\n"，则使用空格表示；否则'\n'不会显示在屏幕上。而我的实现将'\n'看做了一个可打印字符（用空格表示），
	这样虽然看起来比较怪，但是实现比较简单（后来发现，另一种实现方式也不难）。。。。

*/
#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

#define NULL 0

enum mode{QUIT,CMD,INS,REP};	//quit,command,insert,replace
// 变量
	// char* cfn;					//current file name
	
	int rows, columns;			// the terminal screen's size.rows=24,columns=80
	int screensize;				// = rows*columns+8;

	int editing;				// >0时，处于编辑状态
	enum mode cmd_mode;				// 0=command  1=insert 2=replace

	char *status_buffer;	   	// mesages to the user,一个200字节的空间

	char *text, *end, *textend;	// pointers to the user data in memory
	char *screenbegin;			// index into text[], of top line on the screen
	char *dot;					// where all the action takes place

	int TabLength = 4;			// Tab键的宽度：4
	// int cursorPos;
// 函数声明
	void edit_file(char * fn);
	void doCmd(char c);

	void freeResource();
	void new_text(int size);
	int file_size(char* fn);
	int file_insert(char * fn, char * p, int size);
	void redraw();
	void showStatusLine();
	int countCursorPosFromScreenBegin2Dot();
	void synchronize();

	inline void dot_increase();
	inline void dot_decrease();
	void dot_left();
	void dot_right(void);
	void dot_head();
	void dot_tail();
	int dot_preLine();
	int dot_nextLine();
	int dot_up();
	int dot_down();
	void dot_textStart();
	void dot_textEnd();

	char *begin_line(char * p);
	char *end_line(char * p);
	char *prev_line(char * p);
	char *next_line(char * p);
	void debug(char c);

	void modifyScreenBegin();
	void printOnStatusLine(char* s);

	int isSpace(char c);
	void dot_move2NextWordHead();
	void dot_move2NextWordTail();
	void dot_move2PreWordHead();
	void dot_move2PreWordTail();

// 主函数
int main(int argc, char **argv)
{
	if(argc!=2){
		if(argc==1)
			printf(1,"need a file name behind \"vi\" \n");
		else
			printf(1,"too many arguments!\n");
	}
	else{
		editing = 1;	// 0=exit, 1=one file
		//if (cfn != 0)
		//	free(cfn);
		// cfn =(Byte*) argv[1];
		edit_file(argv[1]);
	}
	exit();
}
void edit_file(char * fn)
{
	// 初始化变量
	rows = 24;
	columns = 80;
	screensize = rows*columns;
	TabLength = 4;

	editing = 1;
	cmd_mode = CMD; // command line  enum mode{QUIT,CMD,INS,REP};
	

	// 状态栏
	if(status_buffer!=NULL){
		free(status_buffer);
	}
	status_buffer = (char*)malloc(128);

	// 初始化text[]
	int size,cnt;
	cnt = file_size(fn);
	size = cnt*2;
	new_text(size);	// 设置text[]
	

	//将文本读入内存
	size = file_insert(fn, text, cnt);
	if(size==-1){
		printf(1,"file does not exist!\n");
		freeResource();
		return;
	}
	
	screenbegin = dot = text;
	end = text + cnt;		// *(end-1)才是最后一个字符

	// 显示界面
	redraw();
	setCursorPos(0,0);
	// cursorPos = 0;

	char c;
	setBufferFlag(0);
	setShowAtOnce(0);
	while(editing>0){
		read(0,&c,1);
		// printf(1,"%d\n",c);
		// if(c=='q')
		// 	break;
		
		doCmd(c);	
		synchronize();
		
		debug(c);
		// keyword *screenbegin cursor *dot 
		//status_buffer[0] = c;
		//status_buffer[1] = ' ';
		//status_buffer[2] = *screenbegin=='\n'?'\\':*screenbegin;
		//status_buffer[3] = ' ';
		//char _cur = getCharOnCursor();
		//status_buffer[4] = _cur=='\n'?'\\':_cur;
		//status_buffer[5] = ' ';
		//status_buffer[6] = *dot;	
		//status_buffer[7] = '\0';
		//showStatusLine();

	}
	setBufferFlag(1);
	setShowAtOnce(1);
	clearScreen();
	setCursorPos(0,0);
}
// keyword *screenbegin cursor *dot 
void debug(char c){
	char _cur = getCharOnCursor();	

	int pos = getCursorPos();
	int i=0;
	for(;i<columns-5;i++)
		writeAt(rows,i,0);
	setCursorPos(rows,0);
	
	char _sb = *screenbegin=='\n'?'\\':*screenbegin;
	
	_cur = (_cur=='\n')?'\\':_cur;
	char _dot = (*dot=='\n')?'\\':*dot;

	//printf(1,"%c",_cur);
	printf(1,"%c scr:%c dot:%c cur:%c pos:(%d,%d) %d | %d ",c,_sb,_dot,_cur,pos/columns,pos%columns,pos,dot-screenbegin);
	
	int cnt = countCursorPosFromScreenBegin2Dot();
	printf(1,"cnt:%d",cnt);
	setCursorPos(pos/80,pos%80);
	return;	

}



// 函数定义

// 函数类型1

	void freeResource(){
		if(status_buffer!=NULL)
			free(status_buffer);
		if(text!=NULL)
			free(text);
	}
	// 创建一个text缓存，存放打开的文件（大小为文件大小的2倍+8）
	void new_text(int size){
		if (size < 10240)
			size = 10240;	// have a minimum size for new files
		if (text != NULL)
			free(text);
		text = (char *) malloc(size + 8);
		memset(text, '\0', size);	// clear new text[]

		textend = text + size;	
		return;
	}
	// 调用接口得到文件大小.
	// -1代表失败，其他代表文件大小
	int file_size(char* fn){
		struct stat st_buf;
		int cnt, sr;
		cnt = -1;
		sr = stat(fn, &st_buf);	// see if file exists
		if (sr >= 0)
			cnt = (int) st_buf.size;
		return cnt;
	}
	// 从文件fn中读入size个字节，插入到指针p处，返回插入字节数
	int file_insert(char * fn, char * p, int size){
		int fd, cnt;
		cnt = -1;

		// 打开文件
		fd = open(fn, O_RDWR);			// assume read & write
		if (fd < 0) {
			// could not open for writing- maybe file is read only
			fd = open((char *) fn, O_RDONLY);	// try read-only
			if (fd < 0) {
				goto fi0;
			}
		}

		cnt = read(fd, p, size);
		close(fd);

		if(cnt < 0)
			cnt = -1;
	  	
	  	fi0:
		return cnt;
	}
	// 刷新显示器。
	void redraw(){
		clearScreen();
		modifyScreenBegin();
		copyFromTextToScreen((char*)screenbegin,0,strlen((char*)screenbegin));
	}
	void modifyScreenBegin(){
		if(dot<screenbegin){
			screenbegin = dot;
			screenbegin = begin_line(screenbegin);
		}
		while(countCursorPosFromScreenBegin2Dot()>=screensize){
			screenbegin = next_line(screenbegin);
		}
	}
	// 显示状态行
	void showStatusLine(){
		int pos = getCursorPos();
		int i=0;
		for(;i<columns-5;i++)
			writeAt(rows,i,0);

		setCursorPos(rows,0);
		printf(1,"%s",status_buffer);

		setCursorPos(pos/80,pos%80);
		return;
	}
	// 计算从screenbegin到dot的相对位移。只考虑可打印字符、'\n','\t'。
	int countCursorPosFromScreenBegin2Dot(){
		if(screenbegin>dot){
			// adjust screenbegin
			screenbegin = begin_line(dot);
		}
		char* p = screenbegin;
		int cnt = 0;

		for(; p<dot; p++){			// 是p<dot,小于，没有等号

			// 判断*p及*(p-1)的相关情况
			if(*p>=' ' && *p<='~'){		// 32-126 可打印字符
				cnt++;
			}		
			else if(*p=='\t'){		// Tab = 4
				cnt += TabLength;
			}
			else if(*p=='\n'){
				cnt++;
				if(cnt%columns!=0)
					cnt = cnt/columns*columns+columns;
			}
			else{
				// show error!
				;
			}
		}
		return cnt;
	}

	// 检查screenbegin是否变化过，以便更新屏幕
	void synchronize(){
		static char* syn_screenbegin = 0;

		modifyScreenBegin();
		if(syn_screenbegin!=screenbegin){
			redraw();	
		}
		int pos = countCursorPosFromScreenBegin2Dot();
		setCursorPos(pos/columns,pos%columns);
	}

// ---------------------------------------------------------
// dot操作
	// 涉及变量：char* text,end,dot(唯一被改变变量).
	// 基础函数：
	// dot_textStart,dot_textEnd不能调用基础函数
	#define true ((int)1)
	#define false ((int)0)
	inline void dot_increase(){
		if(dot<end-1)
			dot++;
	}
	inline void dot_decrease(){
		if(dot>screenbegin)
			dot--;
	}
	void dot_left(){
		if(dot>text && dot[-1]!='\n'){	// && 短路运算符
			dot_decrease();
		}
	}
	void dot_right(void){
		if(dot<end-1 && *dot!='\n'){
			dot_increase();
		}
	}
	void dot_head(){
		while(dot>text && dot[-1]!='\n'){
			dot_decrease();		
		}
	}

	void dot_tail(){
		while(dot<end-1 && *dot!='\n'){
			dot_increase();
		}
	}
	// 上一行开头，如果没有上一行，dot不变。会调整screenbegin
	int dot_preLine(){
		char* p = dot;
		dot_head();
		if(dot>text && dot[-1]=='\n'){
			dot_decrease();
			dot_head();
			// 更新screenbegin和cursorPos
			if(dot<screenbegin){
				screenbegin = dot;
				screenbegin = begin_line(screenbegin);
			}
			return true;
		}
		else{
			dot = p;
			return false;
		}
	}
	// 下一行开头，如果没有下一行，dot不变.会调整screenbegin
	int dot_nextLine(){
		char* p = dot;
		dot_tail();
		if(dot<end-1 && *dot=='\n'){
			dot_increase();
			dot_head();
			if(countCursorPosFromScreenBegin2Dot()>=rows*columns){
				screenbegin = next_line(screenbegin);
			}
			return true;
		}
		else{
			dot = p;
			return false;
		}
	}
	int dot_up(){
		char* p=dot;
		if(!dot_preLine()){
			return false;
		}
		while(p>text && p[-1]!='\n'){
			p--;
			dot_right();
			if(*dot=='\n')
				break;
		}
		return true;
	}
	int dot_down(){
		char* p = dot;
		if(!dot_nextLine()){
			return false;
		}
		while(p>text && p[-1]!='\n'){
			p--;
			dot_right();
			if(*dot=='\n')
				break;
		}
		return true;
	}
	// 跳到文本开头
	void dot_textStart(){
		dot = text;
		screenbegin = text;
	}
	// 跳到文本的最后一个字符位置
	void dot_textEnd(){
		dot = end-1;
		screenbegin = end-1;
		screenbegin = begin_line(screenbegin);
		while(countCursorPosFromScreenBegin2Dot()<screensize)
			screenbegin = prev_line(screenbegin);
		screenbegin = next_line(screenbegin);
	}
	// 移到下一个单词头
	void dot_move2NextWordHead(){
		while(1){
			dot_increase();
			if(dot==end-1)
				break;
			if(isSpace(*(dot-1)) && !isSpace(*dot))
				break;		
		}
	}
	// 移到下一个单词尾
	void dot_move2NextWordTail(){
		dot_move2NextWordHead();
		while(1){
			if(dot==end-1)
				break;
			if(!isSpace(*(dot-1)) && isSpace(*dot)){
				dot_decrease();
				break;
			}
			dot_increase();
		}
	}
	void dot_move2PreWordHead(){
		while(1){
			dot_decrease();
			if(dot==screenbegin)
				break;
			if(!isSpace(*dot) && dot>screenbegin && isSpace(*(dot-1))){
				break;
			}	
		}
	}
	void dot_move2PreWordTail(){
		while(1){
			dot_decrease();
			if(dot==screenbegin)
				break;
			if(!isSpace(*dot) && isSpace(*dot+1))
				break;
		}
	}
// dot操作
// ---------------------------------------------------------


// ---------------------------------------------------------
// char指针操作
	// 返回指针p所在行的行首
	char *begin_line(char * p){
		while (p>text && p[-1]!='\n')
			p--;
		return p;
	}

	// 返回指针p所在行的行尾
	char *end_line(char * p){
		while (p<end-1 && *p != '\n')
			p++;
		return p;
	}
	// 将指针p指向当前行的上一行 行首
	char *prev_line(char * p){
		p = begin_line(p);	
		if (p>text && p[-1]=='\n')
			p--;			
		p = begin_line(p);
		return p;
	}
	// 返回指针p所在行的下一行行首
	char *next_line(char * p){
		p = end_line(p);
		if (p<end-1 && *p=='\n')
			p++;			
		return p;
	}

// char指针操作
// ---------------------------------------------------------

void printOnStatusLine(char* s){
	int pos = getCursorPos();
	setCursorPos(rows,0);
	printf(1,"%s",s);
	setCursorPos(pos/columns,pos%columns);
}
// do_cmd

// 在xv6中，使用read()读取的特殊按键对应的int值
	#define VI_K_UP			226	// -30 cursor key Up
	#define VI_K_DOWN		227	// -29 cursor key Down
	#define VI_K_LEFT		228	// -27 cursor key Left
	#define VI_K_RIGHT		229	// -28 Cursor Key Right
	#define VI_K_PAGEUP		230	// -26 Cursor Key Page Up
	#define VI_K_PAGEDN		231	// -25 Cursor Key Page Down


	#define VI_K_END		225	// -31 Cursor Key End
	#define VI_K_INSERT		232	// -24 Cursor Key Insert
	#define VI_K_DELETE		233	// -23 Cursor Key Insert

	#define VI_K_ESC		27
	#define VI_K_TAB		9
	#define VI_K_ENTER		10

void doCmd(char C){
	unsigned char c = (unsigned char) C;
	int i = 0;
	//int pos = 0;

	switch (c) {
		case VI_K_UP:
		case VI_K_DOWN:
		case VI_K_LEFT:
		case VI_K_RIGHT:
		case VI_K_PAGEUP:
		case VI_K_PAGEDN:

		case VI_K_END:
		
			goto key_cmd_mode;
	}
									// enum mode{QUIT,CMD,INS,REP};
	if(cmd_mode==REP){

	}
	key_cmd_mode:

	switch(c){
	// 光标移动：上下左右，行首、行尾，文件头、文件尾，向上翻页、向下翻页
		case 'k':					// up
		case VI_K_UP:
			dot_up();
			break;
		case 'j':					// down
		case VI_K_DOWN:
		case VI_K_ENTER:
			dot_down();
			break;
		case 'h':					// left
		case VI_K_LEFT:
			dot_left();
			break;
		case 'l':					// right
		case VI_K_RIGHT:
		case ' ':
			dot_right();
			break;	
		case '^':					// line head
			dot_head();
			break;
		case '$':					// line tail
		case VI_K_END:
			dot_tail();
			break;
		case 'G':					// file tail
			dot_textEnd();
			break;
		case 'g':					// file head
			dot_textStart();
			break;

		case VI_K_PAGEUP:			// page up
			for(i=0;i<rows-1;i++)
				dot_preLine();
			break;
		case VI_K_PAGEDN:			// page down
			for(i=0;i<rows-1;i++)
				dot_nextLine();
			break;
		case 'w':					// dot移动到后一个单词的首字符：单词用空格区分。
			dot_move2NextWordHead();
			break;
		case 'b':					// dot移动到前一个单词的首字符
			dot_move2PreWordHead();
			break;
		case 'e':					// dot移动到后一个单词的尾字符：单词用空格区分。
			dot_move2NextWordTail();
			break;
		case 'L':					// 将光标提到屏幕中最后一行的行首
			dot = screenbegin;
			for(i=0;i<rows-1;i++)
				dot_nextLine();
			break;
		case 'M':					// 将光标提到屏幕中间行的行首
			dot = screenbegin;
			for(i=0;i<rows/2;i++)
				dot_nextLine();
			break;
		case 'H':					// 将光标提到屏幕中最上一行的行首
			dot = screenbegin;
			break;

	// 查找命令
		case 'f':					// 向后查找下一个键入的字符				
			
			// break;
		case ';':					// 重复查找f命令后的字符					
			
			break;	
		case '/':

			break;

	// 修改命令
		case '~':						// 如果光标或dot处的是26个英文字符，而转换大小写

			break;
		case 'r':						// 将光标所在字符修改为下一个键入字符

			break;
		case 'R':						// 连续替换命令

			break;
		case 'd':						// 相关的删除命令
			
			break;
		case 'x':						// 删除光标所在处的字符
		case VI_K_DELETE:

			break;
		case 'J':						// 将当前一行和下一行连接起来，也就是去掉当前行的'\n'
			
			break;
		case 'D':						// 删除从光标到行尾的字符（包括光标所在字符，不包括'\n'）
			
			break;
		case '>':						// 在当前行行首添加Tab
			
			break;
		case '<':						// 在当前行行首删除Tab
			
			break;

		
	// 模式切换命令
		case 'o':					// 在当前行的下面插入一个空行，并进入插入模式
			
			break;
		case 'O':					// 在当前行的上面插入一个空行，并进入插入模式
			
			break;
		case 'i':					// 进入插入模式
		case VI_K_INSERT:

			break;
		case 'I':					// dot移到行首再进入插入模式
			
			break;
		case 'a':					// 进入追加模式
			
			break;
		case 'A':					// dot移到行尾再进入插入模式
			
			break;
		case VI_K_ESC:						
			
			break;
		//case '':						
		//	
		//	break;

	// 一些其他命令
		case ':':						
			
			break;
		case 18:						// ctrl-R  force redraw						
			
			break;
		case 7:							// ctrl-G  show current status
			
			break;

	// 退出命令
		case 'q':
			editing = 0;
			break;
		case 'Z':						// ZZ- if modified, {write}; exit

			break;
		default:
			printOnStatusLine("encounter a wrong char !!!!!!");
			// 输入有误

	}
}


int isSpace(char c){
	if(c=='\n' || c=='\t' || c==' ')
		return true;
	else
		return false;
}

