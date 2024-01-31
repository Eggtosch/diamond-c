a = "a" * 1000 + "a"
b = "a" * 1000 + "b"
for i = 0, i < 20000000, i = i + 1 do
	a == b
	a != b
end

nil
