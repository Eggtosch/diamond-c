a = "a" * 1000
b = "b" * 1000
for i = 0, i < 20000000, i = i + 1 do
	a == b
	a != b
end

nil
