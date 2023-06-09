# MergeBin
 Merge multiple binary files into one file

# example
## Specified Address
 <span style="color:#333333">```MergeBin 0x00000000@boot.bin 0x00002000@app.bin [firmware.bin]```</span>  
## Follow Previous File (Start At 0x00000000)
 <span style="color:#333333">```MergeBin +@boot.bin +@app.bin [firmware.bin]```</span>  

# options
option  | description
------------- | -------------
 size (-s)      | Output file size, default is not specified;  
 pad (-f)       | Pad free space with specified byte;  
 help (-h)      | Print usage massages;  