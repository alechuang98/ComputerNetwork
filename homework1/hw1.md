<center><font size="30"><b>Computer Network HW1</b></font></center>
<center><span style="font-weight:light; color:#7a7a7a; font-family:Merriweather;">by b06902034 </span><span style="font-weight:light; color:#7a7a7a; font-family:Noto Serif CJK SC;">黃柏諭</span></center>
---

### Problem 1

<img src="/home/alec/Documents/ComputerNetwork/homework1/udp.png" style="zoom:67%;" />

* Website: www.google.com

* It's a DNS query, it provide the IP address of hostname

### Problem 2

<img src="/home/alec/Documents/ComputerNetwork/homework1/tcp.png" style="zoom:67%;" />

* The port which server use is 2796

### Problem 3

* Only TCP header contains: sequence number, acknowledgement number, flags, window size, urgent pointer
* Only UDP header contains: length
* Both of them contains source/destination port, check sum.

### Problem 4

<img src="/home/alec/Documents/ComputerNetwork/homework1/pwd.png" style="zoom:67%;" />

* Login website: http://www.eyny.com/member.php?mod=logging&action=login

* Attacker might use packet sniffer to get the password

### Other Observation

* When using https instead of http, I cannot find plain text password via wireshark, hence https might be safer.

  