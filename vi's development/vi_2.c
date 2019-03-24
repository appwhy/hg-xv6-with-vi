/*
	2019-3-20
	vi_2
	实现了"h/j/k/l/^/$"浏览文本的功能。
	核心函数int countCursorPosFromScreenBegin2Dot()：根据指针计算从screenbegin到dot的（针对屏幕的）相对位移

	缺点：正常情况下，若一行只有"\n"，则使用空格表示；否则'\n'不会显示在屏幕上。而我的实现将'\n'看做了一个可打印字符（用空格表示），
	这样虽然看起来比较怪，但是实现比较简单。。。。

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

	enum mode editing;				// >0时，处于编辑状态
	int cmd_mode;				// 0=command  1=insert 2=replace

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
	cmd_mode = CMD; // command line
	

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
void doCmd(char c){
	//int pos = 0;
	switch(c){
		case 'h':
			dot_left();
			break;
		case 'l':
			dot_right();
			break;
		case 'k':
			dot_up();
			break;
		case 'j':	
			dot_down();
			break;
		case '^':
			dot_head();
			break;
		case '$':
			dot_tail();
			break;
		case 'q':
			editing = 0;
			break;
		default:
			;
			// 输入有误
	}
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
		copyFromTextToScreen((char*)screenbegin,0,strlen((char*)screenbegin));
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
			;
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
		dot++;
	}
	inline void dot_decrease(){
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

