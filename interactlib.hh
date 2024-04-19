#ifndef __CAVE_INTERACT_LIB_HH
#define __CAVE_INTERACT_LIB_HH
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <istream>

// Only Linux is supported, mainly due to fopencookie()
#ifndef __linux__
#error "Interactor only supports Linux-x86_64"
#endif

namespace cave {

extern "C" void __cave_interactor_swap_stack(struct RegisterContext *from, struct RegisterContext *to);

#ifdef __x86_64__

struct RegisterContext {
	long long rbx;
	long long rsp;
	long long rbp;
	long long r12;
	long long r13;
	long long r14;
	long long r15;
	long long rip; // Does not record real %rip, 0 for ret, 1 for calling entry
	long long xmm[16];

	static RegisterContext create(void *stack, size_t stack_size, void *entry, void *arg) {
		RegisterContext ctx = {};
		ctx.rsp = (long long)stack + stack_size;
		ctx.rbp = (long long)stack + stack_size;
		ctx.rip = 1;
		ctx.r12 = (long long)entry;
		ctx.r13 = (long long)arg;
		return ctx;
	}
} __attribute__((aligned(16)));

__asm__(".text\n\t"
		".type __cave_interactor_swap_stack, @function\n\t"
		"__cave_interactor_swap_stack:\n\t"
		"\n\t"
		"/* save old integer registers */\n\t"
		"movq %rbx, 0(%rdi)\n\t"
		"movq %rsp, 8(%rdi)\n\t"
		"movq %rbp, 16(%rdi)\n\t"
		"movq %r12, 24(%rdi)\n\t"
		"movq %r13, 32(%rdi)\n\t"
		"movq %r14, 40(%rdi)\n\t"
		"movq %r15, 48(%rdi)\n\t"
		"movq $0, 56(%rdi)\n\t"
		"\n\t"
		"/* save old xmm registers */\n\t"
		"movdqa %xmm8, 64(%rdi)\n\t"
		"movdqa %xmm9, 80(%rdi)\n\t"
		"movdqa %xmm10, 96(%rdi)\n\t"
		"movdqa %xmm11, 112(%rdi)\n\t"
		"movdqa %xmm12, 128(%rdi)\n\t"
		"movdqa %xmm13, 144(%rdi)\n\t"
		"movdqa %xmm14, 160(%rdi)\n\t"
		"movdqa %xmm15, 176(%rdi)\n\t"
		"\n\t"
		"/* load new integer registers */\n\t"
		"movq 0(%rsi), %rbx\n\t"
		"movq 8(%rsi), %rsp\n\t"
		"movq 16(%rsi), %rbp\n\t"
		"movq 24(%rsi), %r12\n\t"
		"movq 32(%rsi), %r13\n\t"
		"movq 40(%rsi), %r14\n\t"
		"movq 48(%rsi), %r15\n\t"
		"/* Don't want to load %rip here */\n\t"
		"\n\t"
		"/* load new xmm registers */\n\t"
		"movdqa 64(%rsi), %xmm8\n\t"
		"movdqa 80(%rsi), %xmm9\n\t"
		"movdqa 96(%rsi), %xmm10\n\t"
		"movdqa 112(%rsi), %xmm11\n\t"
		"movdqa 128(%rsi), %xmm12\n\t"
		"movdqa 144(%rsi), %xmm13\n\t"
		"movdqa 160(%rsi), %xmm14\n\t"
		"movdqa 176(%rsi), %xmm15\n\t"
		"\n\t"
		"/* if %rip is set, jump to it */\n\t"
		"cmpq $0, 56(%rsi)\n\t"
		"je __cave_interactor_swap_done\n\t"
		"jmp __cave_interactor_stack_entry\n\t"
		"__cave_interactor_swap_done:\n\t"
		"ret\n\t"
		"\n\t"
		".type __cave_interactor_stack_entry, @function\n\t"
		"__cave_interactor_stack_entry:\n\t"
		"endbr64\n\t"
		"movq %r12, %rax\n\t"
		"movq %r13, %rdi\n\t"
		"callq *%rax\n\t"
		"ud2\n\t");
#elifdef __aarch64__

struct RegisterContext {
	long long x[12];  /* x19-x30 caller saved registers */
	long long sp;	  /* stack pointer */
	long long pc;	  // Does not record real pc, 0 for ret, 1 for calling entry
	long long v[128]; /* v8-v15 caller saved registers */

	static RegisterContext create(void *stack, size_t stack_size, void *entry, void *arg) {
		RegisterContext ctx = {};
		ctx.sp = (long long)stack + stack_size;
		ctx.pc = 1;
		ctx.x[0] = (long long)entry;
		ctx.x[1] = (long long)arg;
		return ctx;
	}
} __attribute__((aligned(16)));

__asm__(".text\n\t"
		".type __cave_interactor_swap_stack, %function\n\t"
		"__cave_interactor_swap_stack:\n\t"
		"\n\t"
		"/* save old integer registers */\n\t"
		"stp x19, x20, [x0, 0]\n\t"
		"stp x21, x22, [x0, 16]\n\t"
		"stp x23, x24, [x0, 32]\n\t"
		"stp x25, x26, [x0, 48]\n\t"
		"stp x27, x28, [x0, 64]\n\t"
		"stp x29, x30, [x0, 80]\n\t"
		"mov x2, sp\n\t"
		"stp x2, xzr, [x0, 96]\n\t"
		"\n\t"
		"/* save old v registers */\n\t"
		"stp q8, q9, [x0, 112]\n\t"
		"stp q10, q11, [x0, 144]\n\t"
		"stp q12, q13, [x0, 176]\n\t"
		"stp q14, q15, [x0, 208]\n\t"
		"\n\t"
		"/* load new integer registers */\n\t"
		"ldp x19, x20, [x1, 0]\n\t"
		"ldp x21, x22, [x1, 16]\n\t"
		"ldp x23, x24, [x1, 32]\n\t"
		"ldp x25, x26, [x1, 48]\n\t"
		"ldp x27, x28, [x1, 64]\n\t"
		"ldp x29, x30, [x1, 80]\n\t"
		"ldp x2, x3, [x1, 96]\n\t"
		"mov sp, x2\n\t"
		"\n\t"
		"/* load new v registers */\n\t"
		"ldp q8, q9, [x1, 112]\n\t"
		"ldp q10, q11, [x1, 144]\n\t"
		"ldp q12, q13, [x1, 176]\n\t"
		"ldp q14, q15, [x1, 208]\n\t"
		"\n\t"
		"/* if pc is set, jump to it */\n\t"
		"cbz x3, __cave_interactor_swap_done\n\t"
		"b __cave_interactor_stack_entry\n\t"
		"__cave_interactor_swap_done:\n\t"
		"ret\n\t"
		"\n\t"
		".type __cave_interactor_swap_stack, %function\n\t"
		"__cave_interactor_stack_entry:\n\t"
		"mov x1, x19\n\t"
		"mov x0, x20\n\t"
		"blr x1\n\t"
		"udf #0\n\t");

#elif

#error "Unknown architecture, only x86_64 and aarch64 are supported"

#endif

class Interactor: std::iostream {
  private:
	class Pipe {
		class LineLogger {
		  private:
			const char *role;
			char *buffer;
			size_t capacity;
			size_t length;

		  private:
			void append(const char *buf, size_t len) {
				if (len > capacity - length) {
					size_t new_capacity = std::max(capacity * 2, length + len);
					char *new_buffer = (char *)realloc(buffer, new_capacity);
					if (new_buffer == nullptr) {
						fprintf(stderr, "Interactor Fatal Error: line logger buffer realloc failed");
						abort();
					}
					buffer = new_buffer;
					capacity = new_capacity;
				}
				memcpy(buffer + length, buf, len);
				length += len;
			}

			size_t handle(const char *buf, size_t len) {
				ssize_t lf = -1;
				for (size_t i = 0; i < len; i++) {
					if (buf[i] == '\n') {
						lf = i;
						break;
					}
				}
				if (lf == -1) {
					append(buf, len);
					return len;
				}
				append(buf, lf + 1);
				buffer[length - 1] = '\0';
				fprintf(stderr, "[%s]: %s\n", role, buffer);
				length = 0;
				return lf + 1;
			}

		  public:
			LineLogger(const char *role) : role(role), buffer((char *)malloc(1 << 10)), capacity(1 << 10), length(0) {
				if (buffer == nullptr) {
					fprintf(stderr, "Interactor Fatal Error: line logger buffer malloc failed");
				}
			}

			~LineLogger() {
				if (buffer != nullptr) {
					free(buffer);
				}
			}

			void write(const char *buf, size_t len) {
				size_t p = 0;
				while (p < len) {
					p += handle(buf + p, len - p);
				}
			}
		};

	  private:
		size_t capacity;
		size_t head;
		size_t length;
		char *buffer;
		long long time_stamp;
		bool read_closed;
		bool write_closed;
		FILE *read_file = nullptr, *write_file = nullptr;
		const char *role;
		LineLogger logger;
		bool spy;

		void ensure_capacity(size_t new_capacity) {
			if (new_capacity <= capacity) {
				return;
			}
			if (new_capacity < 2 * capacity) {
				new_capacity = 2 * capacity;
			}
			char *new_buffer = (char *)realloc(buffer, new_capacity);
			if (new_buffer == nullptr) {
				fprintf(stderr, "Interactor Fatal Error: buffer realloc failed\n");
				abort();
			}
			if (head + length > capacity) {
				memcpy(new_buffer + capacity, new_buffer, head + length - capacity);
			}
			buffer = new_buffer;
			head = 0;
			capacity = new_capacity;
		}

	  protected:
		/* Call program to produce more data, used if buffer is empty */
		virtual void produce() = 0;
		/* Check if the program is reading outside produce(), because it's a deadlock if produce() itself reads */
		virtual void check_read_context() = 0;

	  public:
		Pipe(const char *role, bool spy)
			: capacity(1 << 10), head(0), length(0), buffer((char *)malloc(1 << 10)), time_stamp(0), read_closed(false),
			  write_closed(false), role(role), logger(role), spy(spy) {
			if (buffer == nullptr) {
				fprintf(stderr, "Interactor Fatal Error: buffer malloc failed\n");
				abort();
			}
		}

		~Pipe() {
			if (read_file != nullptr) {
				fclose(read_file);
			}
			if (write_file != nullptr) {
				fclose(write_file);
			}
			if (buffer != nullptr) {
				free(buffer);
			}
		}

		void close_read() {
			read_closed = true;
			if (buffer != nullptr) {
				free(buffer);
				buffer = nullptr;
			}
		}

		void close_write() { write_closed = true; }

		ssize_t read(char *buf, size_t max_length) {
			if (read_closed) {
				fprintf(stderr, "Interactor Fatal Error: read from closed pipe\n");
				abort();
			}
			check_read_context();
			if (max_length == 0) {
				return 0;
			}
			time_stamp++;
			if (length == 0) {
				if (write_closed) {
					return EOF;
				} else {
					long long old_time_stamp = time_stamp;
					produce();
					if (write_closed && length == 0) {
						return EOF;
					}
					if (old_time_stamp == time_stamp) {
						fprintf(stderr, "[Lib]: Incorrect, %s did not say anything, but the other side is reading\n", role);
						abort();
					}
				}
			}
			size_t p = 0;
			size_t rest = std::min(std::min(max_length, capacity - head), length);
			memcpy(buf + p, buffer + head, rest);
			head = (head + rest) % capacity;
			length -= rest;
			p += rest;
			if (p < max_length && length > 0) {
				size_t rest = std::min(max_length - p, length);
				memcpy(buf + p, buffer + head, rest);
				head = (head + rest) % capacity;
				length -= rest;
				p += rest;
			}
			return p;
		}

		void write(const char *buf, size_t len) {
			if (write_closed) {
				fprintf(stderr, "Interactor Fatal Error: write to closed pipe\n");
				abort();
			}
			if (spy) {
				logger.write(buf, len);
			}
			if (len == 0) {
				return;
			}
			time_stamp++;
			if (read_closed) { // Nobody is reading, so don't write
				return;
			}
			ensure_capacity(length + len);
			size_t part1 = std::min(len, capacity - (head + length) % capacity);
			memcpy(buffer + (head + length) % capacity, buf, part1);
			if (part1 < len) {
				memcpy(buffer, buf + part1, len - part1);
			}
			length += len;
		}

	  private:
		FILE *make_read_file() {
			if (read_closed) {
				return nullptr;
			}
			return fopencookie(
				this, "r",
				{
					.read = [](void *cookie, char *buf, size_t size) { return ((Pipe *)cookie)->read(buf, size); },
					.write = nullptr,
					.seek = nullptr,
					.close = [](void *) { return 0; },
				});
		}

		FILE *make_write_file() {
			if (write_closed) {
				return nullptr;
			}
			return fopencookie(this, "w",
							   {
								   .read = nullptr,
								   .write = [](void *cookie, const char *buf, size_t size) -> ssize_t {
									   ((Pipe *)cookie)->write(buf, size);
									   return size;
								   },
								   .seek = nullptr,
								   .close = [](void *) { return 0; },
							   });
		}

	  public:
		class PipeStreamBuf : public std::streambuf {
		  private:
			Pipe *read_pipe;
			Pipe *write_pipe;
			char get_buf[128]; // Does not need to be large, since the write target is inside the program, no system call
			char put_buf[128];

		  protected:
			int_type underflow() override {
				if (read_pipe == nullptr) {
					return traits_type::eof();
				}
				if (gptr() < egptr()) {
					return traits_type::to_int_type(*gptr());
				}
				ssize_t len = read_pipe->read(get_buf, sizeof(get_buf));
				if (len <= 0) {
					return traits_type::eof();
				}
				setg(get_buf, get_buf, get_buf + len);
				return traits_type::to_int_type(*gptr());
			}

			int_type sync() override {
				if (write_pipe == nullptr) {
					setp(put_buf, put_buf + sizeof(put_buf));
					return 0;
				}
				if (pptr() == pbase()) {
					return 0;
				}
				write_pipe->write(pbase(), pptr() - pbase());
				setp(put_buf, put_buf + sizeof(put_buf));
				return 0;
			}

			int_type overflow(int_type c) override {
				if (c != traits_type::eof()) {
					*pptr() = traits_type::to_char_type(c);
					pbump(1);
				}
				sync();
				return traits_type::not_eof(c);
			}

		  public:
			PipeStreamBuf(Pipe *read_pipe, Pipe *write_pipe) : read_pipe(read_pipe), write_pipe(write_pipe) {
				setg(get_buf, get_buf, get_buf);
				setp(put_buf, put_buf + sizeof(put_buf));
			}
		};

	  public:
		FILE *get_read_file() { // Please don't close the file
			if (read_file == nullptr) {
				read_file = make_read_file();
			}
			return read_file;
		}

		FILE *get_write_file() { // Please don't close the file
			if (write_file == nullptr) {
				write_file = make_write_file();
			}
			return write_file;
		}
	};

  public:
	constexpr static int REPLACE_STDIO = 1;
	constexpr static int REPLACE_IOSTREAM = 2;
	constexpr static int SPY_PIPES = 4;
	constexpr static int COUNT_INTERACTION = 8;
	constexpr static int ALL = REPLACE_STDIO | REPLACE_IOSTREAM | SPY_PIPES | COUNT_INTERACTION;
	constexpr static size_t SLAVE_STACK_SIZE = 1 << 20;

  private:
	void *slave_stack = nullptr;
	bool slave_running = false;
	bool slave_exited = false;
	RegisterContext master = {}, slave = {};
	FILE *sys_in, *sys_out;
	std::streambuf *sys_in_buf, *sys_out_buf, *buf;
	int flags = 0;
	ssize_t interaction_count = 0;
	ssize_t max_interaction_count = -1;

  public:
	class Agent : public std::iostream {
	  private:
		/* interactor */
		class Interactor *interactor;

	  public:
		/* interact with master */
		FILE *in, *out;
		/* system streams */
		FILE *sys_in, *sys_out;
		std::istream sys_cin;
		std::ostream sys_cout;

		ssize_t count() { return interactor->interaction_count; }

		void set_max_count(ssize_t count) {
			if (count <= -1) {
				fprintf(stderr, "Interactor Fatal Error: set_max_count(%zd)\n", count);
				abort();
			}
			interactor->max_interaction_count = count;
		}

		Agent(Pipe *master_to_slave, Pipe *slave_to_master, FILE *sys_in, FILE *sys_out, std::streambuf *sys_in_buf,
			  std::streambuf *sys_out_buf, class Interactor *interactor)
			: std::iostream(new Pipe::PipeStreamBuf(master_to_slave, slave_to_master)), interactor(interactor),
			  in(master_to_slave->get_read_file()), out(slave_to_master->get_write_file()), sys_in(sys_in), sys_out(sys_out),
			  sys_cin(sys_in_buf), sys_cout(sys_out_buf) {}
		~Agent() { delete rdbuf(); }
	};

  private:
	std::function<void(Agent &)> slave_entry;
	class MasterToSlave : public Pipe {
	  private:
		Interactor *interactor;

	  protected:
		void produce() override { // Slave reads from master, so wake up master to produce more data
			interactor->slave_running = false;
			__cave_interactor_swap_stack(&interactor->slave, &interactor->master);
		}

		void check_read_context() override {
			if (!interactor->slave_running) {
				fprintf(stderr, "Interactor Fatal Error: read inside produce()");
				abort();
			}
		}

	  public:
		MasterToSlave(Interactor *interactor) : Pipe("Sol", interactor->flags & SPY_PIPES), interactor(interactor) {}
	};

	class SlaveToMaster : public Pipe {
	  private:
		Interactor *interactor;

	  protected:
		void produce() override { // Master reads from slave, so wake up slave to produce more data
			interactor->interaction_count++;
			if (interactor->max_interaction_count != -1 && interactor->interaction_count > interactor->max_interaction_count) {
				fprintf(stderr, "[Lib]: Incorrect, interaction count exceeds limit %zd\n", interactor->max_interaction_count);
				abort();
			}
			interactor->slave_running = true;
			__cave_interactor_swap_stack(&interactor->master, &interactor->slave);
		}

		void check_read_context() override {
			if (interactor->slave_running) {
				fprintf(stderr, "Interactor Fatal Error: read interactor output inside interactor\n");
				fprintf(stderr, "Are you using stdin/stdout in interactor context?\n");
				fprintf(stderr, "This is bad, read from agent.sys_in instead\n");
				abort();
			}
		}

	  public:
		SlaveToMaster(Interactor *interactor) : Pipe("Int", interactor->flags & SPY_PIPES), interactor(interactor) {}
	};

	MasterToSlave master_to_slave = MasterToSlave(this);
	SlaveToMaster slave_to_master = SlaveToMaster(this);

	static void slave_entry_proxy(Interactor *interactor) {
		Agent agent = Agent(&interactor->master_to_slave, &interactor->slave_to_master, interactor->sys_in, interactor->sys_out,
							interactor->sys_in_buf, interactor->sys_out_buf, interactor);
		interactor->slave_entry(agent);
		interactor->slave_to_master.close_write();
		interactor->master_to_slave.close_read();
		interactor->slave_exited = true;
		while (true) {
			interactor->slave_running = false;
			__cave_interactor_swap_stack(&interactor->slave, &interactor->master);
		}
	}

  public:
	Interactor(std::function<void(Agent &)> slave_entry, int flags = ALL)
		: master(), slave(), flags(flags), interaction_count(0), slave_entry(slave_entry) {
		slave_stack = malloc(SLAVE_STACK_SIZE);
		if (slave_stack == nullptr) {
			fprintf(stderr, "Interactor Fatal Error: stack malloc failed\n");
			abort();
		}
		slave = RegisterContext::create(slave_stack, SLAVE_STACK_SIZE, (void *)slave_entry_proxy, this);
		sys_in = stdin;
		sys_out = stdout;
		if (flags & REPLACE_STDIO) {
			stdin = slave_to_master.get_read_file();
			stdout = master_to_slave.get_write_file();
		}
		sys_in_buf = std::cin.rdbuf();
		sys_out_buf = std::cout.rdbuf();
		buf = new Pipe::PipeStreamBuf(&slave_to_master, &master_to_slave);
		if (flags & REPLACE_IOSTREAM) {
			std::cin.rdbuf(buf);
			std::cout.rdbuf(buf);
		}
	}

	FILE *read_file() { return slave_to_master.get_read_file(); }
	FILE *write_file() { return master_to_slave.get_write_file(); }

	~Interactor() {
		if (!slave_exited) {
			fflush(master_to_slave.get_write_file());
			buf->pubsync();
			if (slave_running) {
				fprintf(stderr, "Interactor Fatal Error: Freeing interactor in slave context\n");
				abort();
			}
			slave_running = true;
			__cave_interactor_swap_stack(&master, &slave);
			if (!slave_exited) {
				fprintf(stderr, "[Lib] Incorrect: Solution attempt to exit and leave interactor running\n");
				abort();
			}
		}
		if (slave_stack != nullptr) {
			free(slave_stack);
		}
		if (flags & REPLACE_STDIO) {
			stdin = sys_in;
			stdout = sys_out;
		}
		if (flags & REPLACE_IOSTREAM) {
			std::cin.rdbuf(sys_in_buf);
			std::cout.rdbuf(sys_out_buf);
		}
		delete buf;
		if (flags & COUNT_INTERACTION) {
			fprintf(stderr, "[Lib] Interaction count: %zu\n", interaction_count);
		}
	}

	Interactor(const Interactor &) = delete;
	Interactor &operator=(const Interactor &) = delete;
	Interactor(Interactor &&) = delete;
	Interactor &operator=(Interactor &&) = delete;
};

#define wcin __DO_NOT_USE_WCIN_WITH_INTERACT_HH
#define wcout __DO_NOT_USE_WCOUT_WITH_INTERACT_HH
} // namespace cave
#endif