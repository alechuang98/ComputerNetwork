<center><font size="30"><b>Computer Network HW2</b></font></center>
<center><span style="font-weight:light; color:#7a7a7a; font-family:Merriweather;">by b06902034 </span><span style="font-weight:light; color:#7a7a7a; font-family:Noto Serif CJK SC;">黃柏諭</span></center>
---

### How to use

#### Compile

```
$ make all
```

#### Execute

```
$ ./sender <agent ip> <agent port> <sender ip> <sender port> <file name>
$ ./agent <sender ip> <agent ip> <receiver ip> <sender port> <agent port> <receiver port>
$ ./receiver <agent ip> <agent port> <revceiver ip> <receiver port>
```

not that ip format should be `xxx.xxx.xxx.xxx`, not `local`.

### Sender

The first segment data contains resolution.

```flow
st=>start: Start
op1=>operation: update/initialize threshold
op2=>operation: initialize window size
op3=>operation: add a new data to queue
conda=>condition: queue size < window size?
cond=>condition: ack arrive in time?
send=>operation: send/resend data
update=>operation: update window size
fin=>condition: last ack arrive?
finack=>operation: send fin and wait finack
e=>end

st->op1->op2->conda->send->cond
op3->conda
update->fin
cond(yes)->update
cond(no)->op1
conda(yes)->op3
conda(no)->send
fin(no)->conda
fin(yes)->finack->e

```

### Agent

```flow
st=>start: Start
get=>inputoutput: recv data
drop=>condition: drop it?
where=>condition: is it from sender?
fwsender=>operation: foward to sender
fwreceiver=>operation: foward to revceiver

st->get->drop
drop(yes)->get
drop(no)->where
where(yes)->fwreceiver(right)->get
where(no)->fwsender->get
```

### Receiver

```flow
st=>start: Start
get=>inputoutput: recv data
drop=>condition: data id expected?
exack=>operation: drop and send expected ack
overflow=>condition: buffer overflow?
flush=>operation: flush buffer and play
fin=>condition: is it fin?
add=>operation: add to buffer
finack=>operation: send finack
e=>end: End

st->get->drop
drop(no)->exack(left)->get
drop(yes)->overflow
overflow(yes)->flush->exack
overflow(no)->fin
fin(no)->add->get
fin(yes)->finack->e
```

