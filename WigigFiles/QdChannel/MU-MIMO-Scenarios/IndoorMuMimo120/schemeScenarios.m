AP = [2, 9.5, 1.5];
PAA = [0, 0, -0.07;
    0, 0, 0.07];

TX  = AP + PAA;
STA1 = [8,  12.9641, 1.5];
STA2 = [8, 6.0359, 1.5];

figure,
subplot(1,2,1)
plot3(TX(:,1),TX(:,2),TX(:,3), 'o', 'MarkerFaceColor', 'b')

hold on
plot3(STA1(:,1),STA1(:,2),STA1(:,3), 'ro', 'MarkerFaceColor', 'r')
plot3(STA2(:,1),STA2(:,2),STA2(:,3), 'ro', 'MarkerFaceColor', 'r')

axis([0 10 0 19 0 3])
xlabel('x (m)')
ylabel('y (m)')
zlabel('z (m)')
grid on

subplot(1,2,2)
plot3(TX(:,1),TX(:,2),TX(:,3), 'o', 'MarkerFaceColor', 'b')

hold on
plot3(STA1(:,1),STA1(:,2),STA1(:,3), 'ro', 'MarkerFaceColor', 'r')
plot3(STA2(:,1),STA2(:,2),STA2(:,3), 'ro', 'MarkerFaceColor', 'r')

axis([0 10 0 19 0 3])
xlabel('x (m)')
ylabel('y (m)')
zlabel('z (m)')
legend('AP', 'STA', 'Location', 'best')
grid on
view([0 90])

hf = gcf;
hf.Position(3)=hf.Position(3)*2;

AP = [2, 9.5, 1.5];
PAA = 4*[0, -0.07,0;
    0,  0.07, 0];

TX  = AP + PAA;
STA1 = [8,  15.5, 1.5];
STA2 = [8, 3.5, 1.5];

figure,
subplot(1,2,1)
plot3(TX(:,1),TX(:,2),TX(:,3), 'o', 'MarkerFaceColor', 'b')

hold on
plot3(STA1(:,1),STA1(:,2),STA1(:,3), 'ro', 'MarkerFaceColor', 'r')
plot3(STA2(:,1),STA2(:,2),STA2(:,3), 'ro', 'MarkerFaceColor', 'r')

axis([0 10 0 19 0 3])
xlabel('x (m)')
ylabel('y (m)')
zlabel('z (m)')
grid on

subplot(1,2,2)
plot3(TX(:,1),TX(:,2),TX(:,3), 'o', 'MarkerFaceColor', 'b')

hold on
plot3(STA1(:,1),STA1(:,2),STA1(:,3), 'ro', 'MarkerFaceColor', 'r')
plot3(STA2(:,1),STA2(:,2),STA2(:,3), 'ro', 'MarkerFaceColor', 'r')

axis([0 10 0 19 0 3])
xlabel('x (m)')
ylabel('y (m)')
zlabel('z (m)')
legend('AP', 'STA', 'Location', 'best')
grid on
view([0 90])

hf = gcf;
hf.Position(3)=hf.Position(3)*2;


