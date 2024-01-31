n = 100000000
sum = 0.0
flip = -1.0

for i = 1, i <= n, i = i + 1 do
	flip = flip * -1.0
	sum = sum + (flip / (2 * i - 1))
end

sum * 4.0
