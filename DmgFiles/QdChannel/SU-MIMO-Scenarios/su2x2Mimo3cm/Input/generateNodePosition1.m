Dx = 0.1;
x = (4:-Dx:1+Dx).';
pos = [x ([5 1.5]+zeros(length(x),2))];
writematrix(pos, 'NodePosition1.dat');