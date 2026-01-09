file = io.open("C:/Users/twiggo/Desktop/newblob.bin", "wb")

dolphin.hook_instruction(0x8078DDFC, function()
	for i = 0, 0xF, 1 do
		file:write(dolphin.mem_read(0x80882010 + i, 1))
	end
end)