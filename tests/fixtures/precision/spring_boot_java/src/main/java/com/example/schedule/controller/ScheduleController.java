package com.example.schedule.controller;

import com.example.schedule.service.GradeService;
import com.example.schedule.service.LoginService;
import org.springframework.web.bind.annotation.RestController;

@RestController
public class ScheduleController {
    private final LoginService loginService;
    private final GradeService gradeService;

    public ScheduleController(LoginService loginService, GradeService gradeService) {
        this.loginService = loginService;
        this.gradeService = gradeService;
    }
}
