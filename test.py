a = 1
b = 1
i = 0
ret = [str(a), str(b)]
while i < 8:
  c = a + b
  ret.append(str(c))
  a = b
  b = c
  i += 1
print (", ".join(ret))

