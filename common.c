#include "common.h"
#include "libUseful-2.0/libUseful.h"
#include <ctype.h>

int Flags=0;


char *VCatStr(char *Dest, const char *Str1,  va_list args)
{
//initialize these to keep valgrind happy
size_t len=0, ilen;
char *ptr=NULL;
const char *sptr=NULL;


if (Dest !=NULL) 
{
len=StrLen(Dest);
ptr=Dest;
}
else ptr=(char *) calloc(10,1);

if (! Str1) return(ptr);

for (sptr=Str1; sptr !=NULL; sptr=va_arg(args,const char *))
{
	ilen=StrLen(sptr);
	if (ilen > 0)
	{
	ptr=(char *) realloc(ptr,len+ilen+10);
	if (ptr) 
	{
		memcpy(ptr+len, sptr, ilen);
		len+=ilen;
	}
	}
}
ptr[len]='\0';

return(ptr);
}

char *MCatStr(char *Dest, const char *Str1,  ...)
{
char *ptr=NULL;
va_list args;

va_start(args,Str1);
ptr=VCatStr(Dest,Str1,args);
va_end(args);

return(ptr);
}


char *MCopyStr(char *Dest, const char *Str1,  ...)
{
char *ptr=NULL;
va_list args;

ptr=Dest;
if (ptr) *ptr='\0';
va_start(args,Str1);
ptr=VCatStr(ptr,Str1,args);
va_end(args);

return(ptr);
}



char *CatStrLen(char *Dest, const char *Src, size_t len)
{
const char *src, *end;
char *dst;
int dstlen;

dstlen=StrLen(Dest);
Dest=(char *)realloc(Dest,dstlen+len+1);
dst=Dest+dstlen;
src=Src;
end=src+len;
while ((src < end) && (*src != '\0'))
{
*dst=*src;
dst++;
src++;
}
*dst='\0';

return(Dest);
}



char *CatStr(char *Dest, const char *Src)
{
return(CatStrLen(Dest, Src, StrLen(Src)));
}


char *CopyStrLen(char *Dest, const char *Src, size_t len)
{
const char *src, *end;
char *dst;

Dest=(char *)realloc(Dest,len+1);
dst=Dest;
src=Src;
end=src+len;
while ((src < end) && (*src != '\0'))
{
*dst=*src;
dst++;
src++;
}
*dst='\0';

return(Dest);
}


char *CopyStr(char *Dest, const char *Src)
{
if (Dest) *Dest=0;
return(CatStr(Dest,Src));
}



inline char *AddCharToBuffer(char *Dest, size_t DestLen, char Char)
{
char *actb_ptr;

//if (Dest==NULL || ((DestLen % 100)==0)) 
actb_ptr=(char *) realloc((void *) Dest,DestLen +110);
//else actb_ptr=Dest;

actb_ptr[DestLen]=Char;
actb_ptr[DestLen+1]='\0';

return(actb_ptr);
}



//This function looks at a field specifier (list of fields) and extracts the minimum and
//maximum fields
void GetMinMaxFields(const char *FieldSpec, int *MinField, int *MaxField)
{
const char *ptr;
char *end;
int val;

*MinField=0;
*MaxField=0;
if (StrLen(FieldSpec))
{
	ptr=FieldSpec;
	while (isspace(*ptr)) ptr++;
  if (isdigit(*ptr)) 
	{
		*MinField=strtol(FieldSpec, &end, 10);
		ptr=end;
	}

	//will be zero if Field spec didn't start with a number
  val=*MinField;
  while (ptr && (*ptr != '0'))
  {
    if (val > *MaxField) *MaxField=val;
    while ((! isdigit(*ptr)) && (*ptr !='\0'))  ptr++;
    if (! StrLen(ptr)) break;
		if (! isdigit(*ptr)) break;
    val=strtol(ptr,&end,10);
		ptr=end;
		if (val > *MaxField) *MaxField=val;
  }
}


}



void ParseCommandValue(int argc, char *argv[], int pos, int Flag, char **String)
{

if (pos >= argc)
{
	fprintf(stderr,"ERROR: Argument missing after '%s'\n",argv[pos-1]);
	exit(1);
}

Flags |= Flag;
if (String) *String=DeQuoteStr(*String,argv[pos]);
}



char *FILEReadLine(char *RetStr, FILE *f, char Term)
{
int inchar, len=0;

RetStr=CopyStr(RetStr,"");
inchar=fgetc(f);
while ((inchar != Term) && (inchar != EOF))
{
	RetStr=AddCharToBuffer(RetStr,len++,inchar & 0xFF);
	inchar=fgetc(f);
}

if ((inchar==EOF) && (StrLen(RetStr)==0))
{
	Destroy(RetStr);
	return(NULL);
}

return(RetStr);
}





#define ESC 0x1B

char *DeQuoteStr(char *Buffer, const char *Line)
{
char *out, *in;
size_t olen=0;
char hex[3];

if (Line==NULL) return(NULL);
out=CopyStr(Buffer,"");
in=(char *) Line;

while(in && (*in != '\0') )
{
	if (*in=='\\')
	{
		in++;
		switch (*in)
		{
		  case 'e': 
			out=AddCharToBuffer(out,olen,ESC);
			olen++;
			break;


		  case 'n': 
			out=AddCharToBuffer(out,olen,'\n');
			olen++;
			break;

		  case 'r': 
			out=AddCharToBuffer(out,olen,'\r');
			olen++;
			break;

		  case 't': 
			out=AddCharToBuffer(out,olen,'\t');
			olen++;
			break;

			case 'x':
			in++; hex[0]=*in;
			in++; hex[1]=*in;
			hex[2]='\0';
			out=AddCharToBuffer(out,olen,strtol(hex,NULL,16) & 0xFF);
			olen++;
			break;

		  case '\\': 
		  default:
			out=AddCharToBuffer(out,olen,*in);
			olen++;
			break;

		}
	}
	else 
	{
		out=AddCharToBuffer(out,olen,*in);
		olen++;
	}
	in++;
}

return(out);
}



char *VFormatStr(char *InBuff, const char *InputFmtStr, va_list args)
{
int inc=100, count=1, result=0, FmtLen;
char *Tempstr=NULL, *FmtStr=NULL, *ptr;
va_list argscopy;

Tempstr=InBuff;

//Take a copy of the supplied Format string and change it.
//Do not allow '%n', it's useable for exploits

FmtLen=StrLen(InputFmtStr);
FmtStr=CopyStr(FmtStr,InputFmtStr);

//Deny %n. Replace it with %x which prints out the value of any supplied argument
ptr=strstr(FmtStr,"%n");
while (ptr)
{
  memcpy(ptr,"%x",2);
  ptr++;
  ptr=strstr(ptr,"%n");
}


inc=4 * FmtLen; //this should be a good average
for (count=1; count < 100; count++)
{
	result=inc * count +1;
  Tempstr=realloc(Tempstr, result+1);

		//the vsnprintf function DESTROYS the arg list that is passed to it.
		//This is just plain WRONG, it's a long-standing bug. The solution is to
		//us va_copy to make a new one every time and pass that in.
		va_copy(argscopy,args);
     result=vsnprintf(Tempstr,result,FmtStr,argscopy);
		va_end(argscopy);

  /* old style returns -1 to say couldn't fit data into buffer.. so we */
  /* have to guess again */
  if (result==-1) continue;

  /* new style returns how long buffer should have been.. so we resize it */
  if (result > (inc * count))
  {
    Tempstr=realloc(Tempstr, result+10);
		va_copy(argscopy,args);
    result=vsnprintf(Tempstr,result+10,FmtStr,argscopy);
		va_end(argscopy);
  }

   break;
}

Destroy(FmtStr);

return(Tempstr);
}



char *FormatStr(char *InBuff, const char *FmtStr, ...)
{
char *tempstr=NULL;
va_list args;

va_start(args,FmtStr);
tempstr=VFormatStr(InBuff,FmtStr,args);
va_end(args);
return(tempstr);
}


void StripTrailingWhitespace(char *str)
{
size_t len;
char *ptr;

len=StrLen(str);
if (len==0) return;
for(ptr=str+len-1; (ptr >= str) && isspace(*ptr); ptr--) *ptr='\0';
}

void StripCRLF(char *Line)
{
size_t len;
char *ptr;

len=StrLen(Line);
if (len < 1) return;

for (ptr=Line+len-1; ptr >= Line; ptr--)
{
   if (strchr("\n\r",*ptr)) *ptr='\0';
   else break;
}

}




