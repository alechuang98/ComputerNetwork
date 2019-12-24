<center><font size="30"><b>Computer Network HW2</b></font></center>
<center><span style="font-weight:light; color:#7a7a7a; font-family:Merriweather;">by b06902034 </span><span style="font-weight:light; color:#7a7a7a; font-family:Noto Serif CJK SC;">黃柏諭</span></center>
---

### Problem 1

```sequence
Client->Server: Vedio playing command
Note over Server: Change the status tag of client to vedio playing
Note over Client: recv() for vedio resolution
Server->Client: The resolution of vedio
Note over Client: recv() for vedio data with size of data per frame
Server->Client: Keep sending vedio data per frame
Client-->Server: Request for END_TAG if client want to stop streaming
Note over Client: recv() data until getting END_TAG
Server->Client: Sending END_TAG of vedio
Note over Client: End of vedio
Note over Server: Change the status tag of client to default
```

在server端接受到指令時，會先判斷檔案是否可播放，若不可就會回傳錯誤，若可以就會回傳解析度。client根據接收到的資料進行後續處理。

影片播放時定義一個END_TAG，在client讀到一段資料開頭是END_TAG時就結束播放。若client想中斷播放，client端會立即中斷影片播放，並且對server要求送出END_TAG，client會持續讀到END_TAG為止。

### Problem 2

Upload:

```sequence
Client->Server: File upload command with file name
Note over Server: Change the status tag of client to upload
Note over Server: Open the file and recv() for file size 
Client->Server: The size of file
Note over Server: recv() for file data 
Client->Server: Keep sending file data
Note over Server: Close the file
Note over Server: Change the status tag of client to default
```

上傳前會先在client本地判斷是否存在檔案，若存在才會進行上圖的資料交換流程。

Download:

```sequence
Client->Server: File download command with file name
Note over Server: Change the status tag of client to upload
Note over Server: Open the file if the file exist
Server->Client: Sending the size of file or the file doesn't exist
Note over Client: recv() the size of file or the file doesn't exist
Note over Client: Open the file if the file exist
Server-->Client: Keep sending the file data if the file exist
Note over Server: Close the file if the file exist
Note over Server: Change the status tag of client to default
```

若server回傳的檔案大小第一個字元不是數字，就視為檔案不存在，就不會執行後續指令。

### Problem 3

若connect斷開之後持續要求讀寫，就會收到SIGPIPE訊號。正常情況下SIGPIPE不會在我的程式裡發生，除非是刻意在傳輸過程中中斷連線。如過會發生的話寫個handler捕捉訊號並將該client視為斷線不要再進行I/O就可以解決了。

### Problem 4

blocking I/O 不等於 synchronized I/O。

假設老闆(Process)請一位員工(Kernel)處理一份文件。

老闆是否要等員工而能不能做其他事是blocking or non-blocking。

而員工在處理文件時能不能做其他事還是要持續匯報情況是synchronized or asynchronized。