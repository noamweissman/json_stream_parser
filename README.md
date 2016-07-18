# json_stream_parser
A small JSON stream parser

Why have I written my own JSON parser?

There are many JSON parsers so why not use one that was available parsers?

I am an embedded programmer, In the last few years I am working with LwIP + FreeRTOS on ST microprocessors. 

Today “embedded” systems normally run under Linux with almost unlimited resources. These are not so embedded in relation to resources. Those systems are actually mini computers that have huge amount of RAM, normally a file system and much more.

When we talk about real embedded systems we talk about systems that are short on RAM, do not have a file system, sometimes do not even use an OS and probably do not have any storage or limited in storage.

I checked probably dozens of parsers before I decided to write my own. Almost all the JSON parsers that I checked use lots of RAM. Most of the parsers use 64K sometimes much more or even use lots of memory allocations.

In my projects we have 20K sometimes 64K and with the larger processors we have 128 – 192K RAM. Sometimes we have an external SPI FLASH. 

Due to system limitations we cannot use lots of memory allocation as memory is limited and lots of allocation will cause fragmentations and other deterministic issues. 

My aim was to have a simple JSON stream parser that will need minimum RAM as possible. Data is streamed to the parser in real time.

After checking around and looking at many parsers, I tested two parsers. The first was benejson and the second was JSMN. 

The benejson is nicely written and can probably be used as a stream parser but it is very complicated and has almost no documentation. I played with it for several days and eventually decided to put it aside.

JSMN is a small and very nice parser. It is simple, well written but it does not suit my needs. It forces the user to load the full file to a buffer and on top of it needs a lots or RAM for its tokens. 

Eventually I used portions of JSMN and created my own parser. The code is simple, and uses just a few hundred bytes of RAM. If I compare it to 64K RAM or even more, then how is that achievable. ?

Well it uses a small buffer to collects characters and a call back function that handles the data in that small buffer. The buffer size is defined from the user needs, meaning the largest key or data value.

The buffer is used to store keys or value. As JSON data can be nested (an object inside an object) a small stack is used to keep the nesting states. Data is parsed on the fly in chunks. Data chunks can be just a few bytes each.

The parser itself is just an Engine that calls a user defined callback function. The user defined callback function is actually doing the parsing of the data according to a predefined file structure. In other words the engine is general and the user is actually creating the handling of the data specifically for every project.

This is good for small systems that use JSON as a configuration file or as streaming data for queries etc…
