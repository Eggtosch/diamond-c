a = "a" * 1000
b = "a" * 1001
for i = 0, i < 50000000, i = i + 1 do
	a == b
	a != b
end

nil
