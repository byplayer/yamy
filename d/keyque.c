///////////////////////////////////////////////////////////////////////////////
// keyque.c


///////////////////////////////////////////////////////////////////////////////
// Definitions


typedef struct KeyQue
{
  ULONG count;			// Number of keys in the que
  ULONG lengthof_que;		// Length of que
  KEYBOARD_INPUT_DATA *insert;	// Insertion pointer for que
  KEYBOARD_INPUT_DATA *remove;	// Removal pointer for que
  KEYBOARD_INPUT_DATA *que;
} KeyQue;


#define KeyQueSize 100


///////////////////////////////////////////////////////////////////////////////
// Prototypes


NTSTATUS KqInitialize(KeyQue *kq);
void KqClear(KeyQue *kq);
NTSTATUS KqFinalize(KeyQue *kq);
BOOLEAN KqIsEmpty(KeyQue *kq);
ULONG KqEnque(KeyQue *kq, IN  KEYBOARD_INPUT_DATA *buf, IN ULONG lengthof_buf);
ULONG KqDeque(KeyQue *kq, OUT KEYBOARD_INPUT_DATA *buf, IN ULONG lengthof_buf);


#ifdef ALLOC_PRAGMA
#pragma alloc_text( init, KqInitialize )
#pragma alloc_text( page, KqFinalize )
#endif // ALLOC_PRAGMA


///////////////////////////////////////////////////////////////////////////////
// Functions


NTSTATUS KqInitialize(KeyQue *kq)
{
  kq->count = 0;
  kq->lengthof_que = KeyQueSize;
  kq->que = ExAllocatePool(NonPagedPool,
			   kq->lengthof_que * sizeof(KEYBOARD_INPUT_DATA));
  kq->insert = kq->que;
  kq->remove = kq->que;
  if (kq->que == NULL)
    return STATUS_INSUFFICIENT_RESOURCES;
  else
    return STATUS_SUCCESS;
}


void KqClear(KeyQue *kq)
{
  kq->count = 0;
  kq->insert = kq->que;
  kq->remove = kq->que;
}


NTSTATUS KqFinalize(KeyQue *kq)
{
  if (kq->que)
    ExFreePool(kq->que);
  return STATUS_SUCCESS;
}


BOOLEAN KqIsEmpty(KeyQue *kq)
{
  return 0 == kq->count;
}


// return: lengthof copied data
ULONG KqEnque(KeyQue *kq, IN KEYBOARD_INPUT_DATA *buf, IN ULONG lengthof_buf)
{
  ULONG rest;
  
  if (kq->lengthof_que - kq->count < lengthof_buf) // overflow
    lengthof_buf = kq->lengthof_que - kq->count; // chop overflowed datum
  if (lengthof_buf <= 0)
    return 0;

  rest = kq->lengthof_que - (kq->insert - kq->que);
  if (rest < lengthof_buf)
  {
    ULONG copy = rest;
    if (0 < copy)
    {
      RtlMoveMemory((PCHAR)kq->insert, (PCHAR)buf,
		    sizeof(KEYBOARD_INPUT_DATA) * copy);
      buf += copy;
    }
    copy = lengthof_buf - copy;
    if (0 < copy)
      RtlMoveMemory((PCHAR)kq->que, (PCHAR)buf,
		    sizeof(KEYBOARD_INPUT_DATA) * copy);
    kq->insert = kq->que + copy;
  }
  else
  {
    RtlMoveMemory((PCHAR)kq->insert, (PCHAR)buf,
		  sizeof(KEYBOARD_INPUT_DATA) * lengthof_buf);
    kq->insert += lengthof_buf;
  }
  kq->count += lengthof_buf;
  return lengthof_buf;
}


// return: lengthof copied data
ULONG KqDeque(KeyQue *kq, OUT KEYBOARD_INPUT_DATA *buf, IN ULONG lengthof_buf)
{
  ULONG rest;
  
  if (kq->count < lengthof_buf)
    lengthof_buf = kq->count;
  if (lengthof_buf <= 0)
    return 0;

  rest = kq->lengthof_que - (kq->remove - kq->que);
  if (rest < lengthof_buf)
  {
    ULONG copy = rest;
    if (0 < copy)
    {
      RtlMoveMemory((PCHAR)buf, (PCHAR)kq->remove,
		    sizeof(KEYBOARD_INPUT_DATA) * copy);
      buf += copy;
    }
    copy = lengthof_buf - copy;
    if (0 < copy)
      RtlMoveMemory((PCHAR)buf, (PCHAR)kq->que,
		    sizeof(KEYBOARD_INPUT_DATA) * copy);
    kq->remove = kq->que + copy;
  }
  else
  {
    RtlMoveMemory((PCHAR)buf, (PCHAR)kq->remove,
		  sizeof(KEYBOARD_INPUT_DATA) * lengthof_buf);
    kq->remove += lengthof_buf;
  }
  kq->count -= lengthof_buf;
  return lengthof_buf;
}
