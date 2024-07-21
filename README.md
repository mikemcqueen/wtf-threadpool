WTF-threadpool

An attempt at writing a threadpool with no mutex, queue, or atomics. Why? WTF, why not.

OK it uses one atomic for a makeshift stop token, but I suppose I could change it to
actually be a stop token.

This pool is designed for use for only a special use case: when you have a quantity of
work to do serially, and thus you depend (somehow) on the result of all of that work
being completed, but you'd like to split the work up and execute it in parallel across
multiple threads/CPUs.

Think of it as a lower-overhead (due to thread re-use), and number-of-thread capped
version of std::async.

Uses C++20 features.
