a = [0] * 1000
b = [0] * 1000
b[999] = 1
for i = 0, i < 500000, i = i + 1 do
	a == b
	a != b
end

nil
