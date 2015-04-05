There are a few Yahoo! Mail IMAP proxies out there (YPOPS!, FreePOPs), but to be honest, I haven't had much luck with either of them.  Additionally, switching webmail interfaces to Asia or Classic or whatever can be a bit of a hassle.


Yahoo! supports IMAP on some mobile devices, and now also allows it for anyone using the Zimbra mail client.  Unfortunately, I'm not too fond of that client and much prefer my Thunderbird; too bad it isn't natively supported.


Luckily, Yahoo's IMAP implementation isn't too far off from what other clients recognize; it simply requires a "ID ("GUID" "1")"  to be issued before logging in.  No clients do that (except for these hacked up versions of Thunderbird), so my workaround was to create a simple IMAP proxy.


This proxy simply takes the commands sent to it by your mail client, passes them on to Yahoo, and relays them back to your client.  It all the while looks for your client to issue a "login" command so that it can inject the "id" command to unlock IMAP access.


This program is written in C and has been tested on Linux and Windows (via Cygwin). Read the release notes in the source for more information.  I haven't written in C for a while, and I know that this program can be improved.  If anyone does so, I'd like to hear from you.