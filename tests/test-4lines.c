int foo(void) { return 42; }

int main(void) {
  int x = foo();
  
#if 0
  int y = 123;
  x += y;
#endif
  
  // No-op: comment
  return x + foo();
}
