#ifndef _LOG_H
#define _LOG_H

#if DBG

// Initiallize logging queue and enqueue "message" as
// first log.
void mayuLogInit(const char *message);

// Finalize logging queue.
void mayuLogTerm(void);

// Enqueue one message to loggin queue.
// Use printf like format to enqueue,
// following types are available.
// %x: (ULONG)unsigned long in hexadecimal
// %d: (ULONG)unsigned long in decimal
// %T: (PUNICODE)pointer to unicode string
// Notice: specifing minimal width such as "%2d"
//         is unavailable yet.
void mayuLogEnque(const char *fmt, ...);

// Dequeue one message from logging queue to "irp".
NTSTATUS mayuLogDeque(PIRP irp);

#endif // DBG

#endif // !_LOG_H
