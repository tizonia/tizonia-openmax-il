.. Tizonia OpenMAX IL documentation


Introduction
============

**Tizonia** is an experimental open source implementation of the Khronos `OpenMAX
IL 1.2 Provisional Specification
<http://www.khronos.org/news/press/khronos-group-releases-openmax-il-1.2-provisional-specification>`_.

The project goals are:

* Alignment to the 1.2 specification, with full support for both *Base* and
  *Interop* profiles, which includes tunnelling and resource management.
  Support for the new 1.2 features and updates:

  * Support for arbitrary buffers (i.e. dynamically allocated vs statically
    allocated as in 1.1.2)
  * Updated state machine, including changes to eliminate potential race
    conditions during tunnelling.
  * Ability to cancel certain pending state transitions.
  * Improved support for component roles.
  * Updated error codes and events.
  * Slaving behaviour of ports.
  * Improved port compatibility checks.
  * IL Core updates, including support for IL Core extensions and explicit
    tunnel tear-down API.

* Good overall user space performance of the base component and plug-ins

  * Reduced use of system calls in general
  * Minimal synchronisation overheads with simple threading model in the base
    component.
  * Small object allocator utility.

* Easy plug-in creation

  * Support for multi-role components.

* OS abstraction layer

  * Memory allocation APIs.
  * Small object allocation API.
  * Dynamic array API.
  * Queue API (synchronised).
  * Priority queue API (non-synchronised).
  * Thread/Task control APIs.
  * Semaphore, Mutex and Condition Variable APIs.
  * Logging API.
  * Universally unique identifier API.

* Support for OpenMAX IL Buffer Sharing (FUTURE)
* Configurable threading model (FUTURE)


