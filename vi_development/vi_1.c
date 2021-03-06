/*
	2019-3-18
	vi_1
	实现功能：只实现了基础的浏览功能，可以移动光标
	bug：存在闪屏问题，不能浏览整个文本（j命令不能执行到文件末尾）。 而且dot和光标所在位置没有正确对应。


*/
#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

typedef unsigned char Byte;
#define NULL 0

int rows, columns;			// the terminal screen's size.rows=24,columns=80
int screensize;				// = rows*colomns+8;
int tabstop;				// Tab键的宽度：8

int editing;				// >0时，处于编辑状态
int cmd_mode;				// 0=command  1=insert 2=replace

Byte *status_buffer;	   	// mesages to the user,一个200字节的空间

Byte *text, *end, *textend;	// pointers to the user data in memory
Byte *screenbegin;			// index into text[], of top line on the screen
Byte *dot;					// where all the action takes place

Byte *cfn;					// 文件名

void edit_file(Byte * fn);
void sync_cur_dot_scr();
void showstatus();

int countScreenBeginToDot();
void freshScreenBegin();
Byte *begin_line(Byte * p);
Byte *end_line(Byte * p);
Byte *prev_line(Byte * p);
Byte *next_line(Byte * p);
void redraw();
void dot_left(void);
void dot_right(void);
void dot_begin(void);
void dot_end(void);
void doCmd(char c);
int file_insert(Byte * fn, Byte * p, int size);
void new_text(int size);
int file_size(Byte* fn);

int main(int argc, char **argv)
{
	if(argc!=2){
		printf(1,"worong cmd!");
	}
	else{
		editing = 1;	// 0=exit, 1=one file
		if (cfn != 0)
			free(cfn);
		cfn =(Byte*) argv[1];
		edit_file(cfn);
	}
	exit();
}
void edit_file(Byte * fn)
{
	// 初始化变量
	rows = 25;
	columns = 80;
	screensize = rows*columns;
	tabstop = 8;

	editing = 1;
	cmd_mode = 0; // command line
	

	if(status_buffer!=NULL){
		free(status_buffer);
	}
	status_buffer = (Byte*)malloc(128);

	// 初始化text[]
	int size,cnt;
	cnt = file_size(fn);
	size = cnt*2;

	new_text(size);	// 设置text[]
	

	//将文本读入内存
	size = file_insert(fn, text, cnt);
	if(size==-1){
		;//错误提示
		return;
	}
	
	screenbegin = dot = text;
	end = text + cnt;

	// 显示界面
	redraw();
	setCursorPos(0,0);

	char c;
	setBufferFlag(0);
	setShowAtOnce(0);
	while(editing>0){
		read(0,&c,1);
		// printf(1,"%d\n",c);
		// if(c=='q')
		// 	break;
		status_buffer[0] = (Byte)c;
		showstatus();

		doCmd(c);	

		status_buffer[0] = (Byte)(c+1);
		showstatus();

		sync_cur_dot_scr();
	}
	setBufferFlag(1);
	setShowAtOnce(1);
	clearScreen();
	setCursorPos(0,0);
}

void showstatus(){
	int pos = getCursorPos();
	writeAt(rows-1,0,(char)status_buffer[0]);
	setCursorPos(pos/80,pos%80);
	return;
}
// 同步光标、dot、屏幕三者间的关系
void sync_cur_dot_scr(){
	int changed = 0;
	Byte* tmp;
	// 同步屏幕与dot
	if(dot<screenbegin){
		screenbegin = dot;
		screenbegin = begin_line(screenbegin);
		changed = 1;
	}
	if(countScreenBeginToDot()>24){
		tmp = dot;
		tmp = end_line(tmp);
		freshScreenBegin(tmp);
		changed = 1;
	}
	// 刷新从screenbegin到screenend的内容。
	if(changed)
		redraw();

	// 同步光标与dot。
	int pos = countScreenBeginToDot();
	if(pos<0)
		pos=0;
	setCursorPos(pos/80,pos%80);
	// printf(1,"debug\n");
}
// 返回从screenbegin到dot的相对偏移（针对屏幕而言）
int countScreenBeginToDot(){
	int pos = 0;
	Byte* p = screenbegin;
	Byte* q = screenbegin;
	q = end_line(q);
	while(1){
		if(q>dot){
			pos += (dot-p);
			break;
		}
		else{
			if((q-p)%80>0){
				pos =  pos + (q-p)/80*80 + 80;
			}
			else
				pos += (q-p);
		}
		p = next_line(p);
		q = next_line(q);
		q = end_line(q);
	}
	return pos;
}
void freshScreenBegin(Byte* tmp){
	end_line(tmp);
	Byte* p = tmp;
	Byte* q = tmp;
	begin_line(q);

	int cnt=0;
	while(cnt<rows){
		if(p-q>80){
			cnt += ((p-q)/80);
			if((p-q)%80>0)
				cnt++;
		}
		else
			cnt++;

		q = prev_line(q);
		p = prev_line(p);
		p = end_line(p);
	}
	screenbegin = q;
}

// 返回指针p所在行的行首
Byte *begin_line(Byte * p){
	while (p > text && p[-1] != '\n')
		p--;			// go to cur line B-o-l
	return (p);
}
// 返回指针p所在行的行尾,不包括'\n'
Byte *end_line(Byte * p){
	while (p < end - 1 && *p != '\n')
		p++;			// go to cur line E-o-l
	return (p);
}
// 将指针p指向当前行的上一行 行首
Byte *prev_line(Byte * p){
	p = begin_line(p);	// goto begining of cur line
	if (p[-1] == '\n' && p > text)
		p--;			// step to prev line
	p = begin_line(p);	// goto begining of prev line
	return (p);
}
// 返回指针p所在行的下一行行首
Byte *next_line(Byte * p){
	p = end_line(p);
	if (*p == '\n' && p < end - 1)
		p++;			// step to next line
	return (p);
}
// 刷新显示器。
void redraw(){
	clearScreen();
	copyFromTextToScreen((char*)screenbegin,0,strlen((char*)screenbegin));
}
// 将dot向左移动1个位置
void dot_left(void){
	if (dot > text && dot[-1] != '\n')
		dot--;
}
// 将dot向右移动1个位置
void dot_right(void){
	if (dot < end - 1 && *dot != '\n')
		dot++;
}
// 将dot移到行首
void dot_begin(void){
	dot = begin_line(dot);	// return pointer to first char cur line
}
// 将dot移到行尾
void dot_end(void){
	dot = end_line(dot);	// return pointer to last char cur line
}
void doCmd(char c){
	switch(c){
		case 'h':
			dot_left();
			break;
		case 'l':
			dot_right();
			break;
		case 'k':
			dot = prev_line(dot);
			break;
		case 'j':	
			dot = next_line(dot);
			break;
		case 'q':
			editing = 0;
			break;
		default:
			;
			// 输入有误
	}
}
// 从文件fn中读入size个字节，插入到指针p处，返回插入字节数
int file_insert(Byte * fn, Byte * p, int size){
	int fd, cnt;
	cnt = -1;

	// 打开文件
	fd = open((char *) fn, O_RDWR);			// assume read & write
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
// 创建一个text缓存，存放打开的文件（大小为文件大小的2倍+8）
void new_text(int size){
	if (size < 10240)
		size = 10240;	// have a minimum size for new files
	if (text != 0)
		free(text);
	text = (Byte *) malloc(size + 8);
	memset(text, '\0', size);	// clear new text[]

	textend = text + size;	
	return;
}
// 调用接口得到文件大小.
// -1代表失败，其他代表文件大小
int file_size(Byte* fn){
	struct stat st_buf;
	int cnt, sr;
	cnt = -1;
	sr = stat((char *) fn, &st_buf);	// see if file exists
	if (sr >= 0)
		cnt = (int) st_buf.size;
	return cnt;
}


