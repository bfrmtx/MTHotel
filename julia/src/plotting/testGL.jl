using GLMakie

# 1. Create a layout scene window
fig = Figure()
ax = Axis(fig[1, 1], title = "Interactive Desktop GUI")

# 2. Add a GUI slider to the layout
ls_amplitude = Slider(fig[2, 1], range = 0.1:0.1:5.0, startvalue = 1.0)

# 3. Create a reactive variable linked to the slider value
amplitude = ls_amplitude.value

# 4. Generate data dynamically based on the slider value
x = range(0, 10, length=500)
y = lift(amplitude) do a
    a .* sin.(x)
end

# 5. Plot the dynamic data
lines!(ax, x, y, color = :blue, linewidth = 3)

# Display the GUI window and block until it's closed (display() itself doesn't wait)
screen = display(fig)
wait(screen)
