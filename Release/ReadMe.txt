2F function name changed to "syscallF" or "syscall3", prevent conflicts with float 2f.
Now using double-byte jump, four-byte are not necessary.

usetemplist() :without any function output, just for making hex() usable with variables.
For example: usetemplist(i1, hex(55020700));

============

2022H301:

The tail will be padded with bytes guaranteed to be 16-bytes aligned.

void Voice2(string s, float f) - the content in brackets will be generated correctly.

Support for setting default values of global variables.
Should be written before void as:
initialize Mission()
{
	gloabl_var1(value);
    gloabl_var2(value);
    gloabl_var...(value);
}
Value support: "int" less than 127, "float" ending with f, "variable length hex" ending with h.
The hex length should be a multiple of 2, and don't use it when in doubt.
Support it because: 'gloabl_var1(1, 0900h);' will be considered by the game as 'gloabl_var1 = -1'